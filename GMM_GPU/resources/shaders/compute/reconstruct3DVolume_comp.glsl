#version 460 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rg32f)		uniform writeonly image3D tex_Volume;

layout(binding = 1)				uniform sampler3D tex_GMMCoeff_1;
layout(binding = 2)				uniform sampler3D tex_GMMCoeff_2;
layout(binding = 3)				uniform sampler3D tex_GMMCoeff_3;
layout(binding = 4)				uniform sampler3D tex_GMMCoeff_4;

float PI = 3.141592653589793238462643383279;

uniform int B;

uniform float vMin;
uniform float vMax;
uniform int NumIntervals;
uniform int NumBricks;

uniform ivec3 O;
uniform ivec3 R;
uniform int BRICK_IDX;

shared float probBuffer[256];

float random(vec2 p)
{
	vec2 K = vec2(
		23.14069263277926, // e^pi (Gelfond's constant)
		2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant)
	);
	return fract(cos(dot(p, K)) * 12345.6789);
}

float random_3(vec3 p)
{
	vec3 K = vec3(
		23.14069263277926, // e^pi (Gelfond's constant)
		2.665144142690225, // 2^sqrt(2) (Gelfondâ€“Schneider constant)
		1.6180339887498948 // Φ
	);
	return fract(cos(dot(p, K)) * 12345.6789);
}

float gaussian1D(float mean, float var2, float x)
{
	float v = 1.0f / sqrt(2.0f * PI * var2);
	return v * exp(-0.5f * (x - mean) * (x - mean) / var2);
}

float gaussian3D(vec3 means, vec3 var2s, vec3 p)
{
	float vx = gaussian1D(means.x, var2s.x, p.x);
	float vy = gaussian1D(means.y, var2s.y, p.y);
	float vz = gaussian1D(means.z, var2s.z, p.z);
	return vx * vy * vz;
}

vec2 evaluateValuePerTex(ivec3 texCoord, vec3 p, sampler3D GMMCoefftex)
{
	ivec3 coordMean = texCoord;
	ivec3 coordVar2 = texCoord + ivec3(0, 0, NumBricks);

	vec4 means = texelFetch(GMMCoefftex, coordMean, 0);
	vec4 var2s = texelFetch(GMMCoefftex, coordVar2, 0);

	float binWeight = means.w;
	float kernelValue = var2s.w * gaussian3D(means.xyz, var2s.xyz, p);

	return vec2(binWeight, kernelValue);
}

float evaluateProb(vec3 p, ivec3 texCoord)
{
	vec2 V1 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_1);
	vec2 V2 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_2);
	vec2 V3 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_3);
	vec2 V4 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_4);

	return V1.x * (V1.y + V2.y + V3.y + V4.y);
}

void main()
{
	ivec3 sp = ivec3(gl_GlobalInvocationID.xyz) / ivec3(NumIntervals, 1, 1);
	ivec3 samplePoint = sp + ivec3(O);

	int binIdx = int(gl_LocalInvocationID.x);

	// step 1. find which brick the sample point locates at

	// step 2. find which block the sample point belongs to
	uvec3 brickOffsetPos = sp;
	uvec3 sampP = brickOffsetPos % uvec3(B);

	uvec3 brickBlockRes = R / uvec3(B);
	uvec3 brickBlockPos = brickOffsetPos / uvec3(B);

	if (brickOffsetPos.x >= brickBlockRes.x * B)
	{
		sampP.x += B;
		brickBlockPos.x--;
	}

	if (brickOffsetPos.y >= brickBlockRes.y * B)
	{
		sampP.y += B;
		brickBlockPos.y--;
	}

	if (brickOffsetPos.z >= brickBlockRes.z * B)
	{
		sampP.z += B;
		brickBlockPos.z--;
	}

	uint blockIdx = brickBlockPos.z * brickBlockRes.y * brickBlockRes.x +
		brickBlockPos.y * brickBlockRes.x + brickBlockPos.x;

	// step 3. calculate texture coordiantes
	ivec3 texCoord = ivec3(blockIdx, binIdx, BRICK_IDX);

	// step 4. update shared buffer
	probBuffer[binIdx] = evaluateProb(vec3(sampP), texCoord);

	barrier();

	float dv = (vMax - vMin) / float(NumIntervals);

	// bi [m, M)
	if (binIdx == 0)
	{
		float p = 0.0f;
		int idx = 0;
		for (int i = 0; i < NumIntervals; ++i)
			if (probBuffer[i] > p)
			{
				p = probBuffer[i];
				idx = i;
			}

		float m = vMin + float(idx) * dv;
		imageStore(tex_Volume, samplePoint, vec4(m));
	}
}