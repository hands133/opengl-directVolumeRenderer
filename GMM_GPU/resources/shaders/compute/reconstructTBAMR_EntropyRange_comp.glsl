#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1) in;

layout(binding = 0, r32i)		uniform coherent iimage2D tex_MinMaxEntropy;	
layout(binding = 1)				uniform sampler3D tex_Entropy;

uniform int currentLevel;
uniform int texW;
uniform int PS;		// patch-size

ivec2 CalCellMinMaxEntropy(vec4 cellInfo)
{
	float w0 = cellInfo.w / float(PS);
	float wOffset = w0 / 2.0f;

	float eMin = 100.0f;
	float eMax = -1.0f;
	
	for (int i = 0; i < PS; ++i)
		for (int j = 0; j < PS; ++j)
			for (int k = 0; k < PS; ++k)
			{
				vec3 pSamp = cellInfo.xyz + vec3(wOffset) + vec3(w0) * vec3(i, j, k);
				float entropy = texture(tex_Entropy, pSamp).x;
	
				if (entropy < eMin)	eMin = entropy;
				if (entropy > eMax)	eMax = entropy;
			}
	
	int eMinInt = int(eMin * 1.0e9f);
	int eMaxInt = int(eMax * 1.0e9f);

	return ivec2(eMinInt, eMaxInt);
}

void main()
{
	int L = currentLevel;
	int N = int(pow(2.0, float(L)));
	float cellW = 1.0f / float(N);

	int seqL = int(gl_GlobalInvocationID.x);
	int z0 = seqL / (N * N);
	seqL -= z0 * (N * N);
	int y0 = seqL / N;
	seqL -= y0 * N;
	int x0 = seqL;

	vec4 cellInfo = vec4(vec3(x0, y0, z0) * vec3(cellW), cellW);
	ivec2 mM = CalCellMinMaxEntropy(cellInfo);

	ivec2 minEIter = ivec2(currentLevel, 0);
	ivec2 maxEIter = ivec2(currentLevel, 1);
	
	imageAtomicMin(tex_MinMaxEntropy, minEIter, mM.x);
	imageAtomicMax(tex_MinMaxEntropy, maxEIter, mM.y);
}