#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)		uniform writeonly image3D tex_Entropy;

layout(binding = 1)				uniform sampler3D tex_GMMCoeff_1;
layout(binding = 2)				uniform sampler3D tex_GMMCoeff_2;
layout(binding = 3)				uniform sampler3D tex_GMMCoeff_3;
layout(binding = 4)				uniform sampler3D tex_GMMCoeff_4;

uniform int B;				// block-size

uniform float vMin;			// value-min
uniform float vMax;			// value-max
uniform int NumIntervals;	// B
uniform int NumBricks;		// NB

uniform ivec3 O;			// Origin
uniform ivec3 R;			// Resolution
uniform int BRICK_IDX;		// BRICK_IDX

float PI = 3.14159265358979323846;
float eps = 1.192092896e-07F;


float I_1(vec4 S1, vec4 S2, vec4 S3, vec4 S4)
{
	float e = 0.0f;

	e += S1.w * (log(S1.x) + log(S1.y) + log(S1.z));
	e += S2.w * (log(S2.x) + log(S2.y) + log(S2.z));
	e += S3.w * (log(S3.x) + log(S3.y) + log(S3.z));
	e += S4.w * (log(S4.x) + log(S4.y) + log(S4.z));

	e *= 3.0 * log(2.0 * PI) / 2.0f;
	return e;
} 

float I_2(float binWeight)
{
	return -log(min(1.0f, binWeight));
}

float I_3(uvec3 BS)
{
	return -log(float(BS.x * BS.y * BS.z));
}

float CalCondEntropy_IL(uint blockIdx, uvec3 BlockRes)
{
	float E_IL = 0.0f;
	float I1 = 0.0f;
	float I2 = 0.0f;
	float I3 = I_3(BlockRes);
	
	for (int binIdx = 0; binIdx < NumIntervals; ++binIdx)
	{
		ivec3 coordMean = ivec3(blockIdx, binIdx, BRICK_IDX);
		ivec3 coordVar2 = coordMean + ivec3(0, 0, NumBricks);

		vec4 means_1 = texelFetch(tex_GMMCoeff_1, coordMean, 0);
		float binWeight = min(means_1.w, 1.0f);
		if (binWeight <= 0.0f)	continue;
		vec4 var2s_1 = texelFetch(tex_GMMCoeff_1, coordVar2, 0);

		vec4 means_2 = texelFetch(tex_GMMCoeff_2, coordMean, 0);
		vec4 var2s_2 = texelFetch(tex_GMMCoeff_2, coordVar2, 0);

		vec4 means_3 = texelFetch(tex_GMMCoeff_3, coordMean, 0);
		vec4 var2s_3 = texelFetch(tex_GMMCoeff_3, coordVar2, 0);

		vec4 means_4 = texelFetch(tex_GMMCoeff_4, coordMean, 0);
		vec4 var2s_4 = texelFetch(tex_GMMCoeff_4, coordVar2, 0);

		vec4 S_1 = vec4(var2s_1.xyz, means_1.w);
		vec4 S_2 = vec4(var2s_2.xyz, means_2.w);
		vec4 S_3 = vec4(var2s_3.xyz, means_3.w);
		vec4 S_4 = vec4(var2s_4.xyz, means_4.w);

		I1 = -I_1(S_1, S_2, S_3, S_4);
		I2 = I_2(binWeight);

		//E_IL += binWeight * (I1 + I2 + I3);
		E_IL += binWeight * I2;		// H(I)
	}

	return E_IL;
}


float gaussian1D(float mean, float var2, float x)
{
	float v = 1.0f / sqrt(2.0f * PI * var2);
	return v * exp(-0.5f * (x - mean) * (x - mean) / var2);
}

float gaussian3D(vec3 means, vec3 var2s, uvec3 p)
{
	float vx = gaussian1D(means.x, var2s.x, float(p.x));
	float vy = gaussian1D(means.y, var2s.y, float(p.y));
	float vz = gaussian1D(means.z, var2s.z, float(p.z));
	return vx * vy * vz;
}

float CalCondEntropy_IfixedL(uint blockIdx, uvec3 BR, uvec3 pos)
{
	float E_IfixedL = 0.0f;
	for (int binIdx = 0; binIdx < NumIntervals; ++binIdx)
	{
		ivec3 coordMean = ivec3(blockIdx, binIdx, BRICK_IDX);
		ivec3 coordVar2 = coordMean + ivec3(0, 0, NumBricks);

		vec4 means_1 = texelFetch(tex_GMMCoeff_1, coordMean, 0);
		vec4 var2s_1 = texelFetch(tex_GMMCoeff_1, coordVar2, 0);

		vec4 means_2 = texelFetch(tex_GMMCoeff_2, coordMean, 0);
		vec4 var2s_2 = texelFetch(tex_GMMCoeff_2, coordVar2, 0);

		vec4 means_3 = texelFetch(tex_GMMCoeff_3, coordMean, 0);
		vec4 var2s_3 = texelFetch(tex_GMMCoeff_3, coordVar2, 0);

		vec4 means_4 = texelFetch(tex_GMMCoeff_4, coordMean, 0);
		vec4 var2s_4 = texelFetch(tex_GMMCoeff_4, coordVar2, 0);

		float binWeight = min(means_1.w, 1.0f);
		if (binWeight == 0.0f)	continue;

		float v1 = var2s_1.w * gaussian3D(means_1.xyz, var2s_1.xyz, pos);
		float v2 = var2s_2.w * gaussian3D(means_2.xyz, var2s_2.xyz, pos);
		float v3 = var2s_3.w * gaussian3D(means_3.xyz, var2s_3.xyz, pos);
		float v4 = var2s_4.w * gaussian3D(means_4.xyz, var2s_4.xyz, pos);

		float vGMM = v1 + v2 + v3 + v4;
		float vTmp = binWeight * vGMM;
		float vPl_bi = vTmp * float(BR.x * BR.y * BR.z);

		float v = binWeight * (v1 + v2 + v3 + v4) * float(BR.x * BR.y * BR.z);
		
		vPl_bi = min(vPl_bi, 1.0f);
		vPl_bi = max(vPl_bi, eps);
		E_IfixedL += vPl_bi * log(vPl_bi);
	}

	return -E_IfixedL;
}

void main()
{
	// step 1. find which brick the sample point locates at
	ivec3 brickOffsetPos = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 samplePoint = brickOffsetPos + ivec3(O);

	// step 2. find which block the sample point belongs to
	uvec3 brickBlockRes = R / uvec3(B);
	uvec3 brickBlockPos = brickOffsetPos / uvec3(B);

	uvec3 sampP = brickOffsetPos % uvec3(B);
	uvec3 blockRes = uvec3(B);

	if (brickOffsetPos.x >= brickBlockRes.x * B)
	{
		brickBlockPos.x--;
		blockRes.x += R.x % B;
		sampP.x += B;
	}

	if (brickOffsetPos.y >= brickBlockRes.y * B)
	{
		brickBlockPos.y--;
		blockRes.y += R.y % B;
		sampP.y += B;
	}

	if (brickOffsetPos.z >= brickBlockRes.z * B)
	{
		brickBlockPos.z--;
		blockRes.z += R.z % B;
		sampP.z += B;
	}

	uint blockIdx = brickBlockPos.z * brickBlockRes.y * brickBlockRes.x +
		brickBlockPos.y * brickBlockRes.x + brickBlockPos.x;

	// step 3. calculate texture coordiantes
	//float Entropy = CalCondEntropy_IL(blockIdx, blockRes);
	float Entropy = CalCondEntropy_IfixedL(blockIdx, blockRes, sampP);
	imageStore(tex_Entropy, samplePoint, vec4(Entropy));
}  