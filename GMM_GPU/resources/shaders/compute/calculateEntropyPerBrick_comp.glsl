#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform writeonly image3D tex_VolumeEntropy;
layout(binding = 1)			uniform sampler3D tex_Volume;

float PI = 3.14159265358979323846;

uniform float vMin;
uniform float vMax;
uniform int NumIntervals;
uniform ivec3 dataRes;
uniform int SS;

uniform ivec3 O;
uniform ivec3 R;


int getValueBin(ivec3 p)
{
	ivec3 pCopy = p;

	if (pCopy.x < 0)	pCopy.x = -pCopy.x;
	if (pCopy.y < 0)	pCopy.y = -pCopy.y;
	if (pCopy.z < 0)	pCopy.z = -pCopy.z;

	if (pCopy.x >= dataRes.x)	pCopy.x = 2 * (dataRes.x - 1) - pCopy.x;
	if (pCopy.y >= dataRes.y)	pCopy.y = 2 * (dataRes.y - 1) - pCopy.y;
	if (pCopy.z >= dataRes.z)	pCopy.z = 2 * (dataRes.z - 1) - pCopy.z;

	float v = texelFetch(tex_Volume, pCopy, 0).x;
	float dv = (vMax - vMin) / float(NumIntervals);

	return int((v - vMin) / dv);
}

void main()
{
	ivec3 localP = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 sampP = localP + O;

	int hist[256];
	for (int i = 0; i < NumIntervals; ++i)	hist[i] = 0;

	int S = SS;

	int lhv = S / 2;
	int rhv = S / 2 + S % 2;

	int xmin = sampP.x - lhv;
	int ymin = sampP.y - lhv;
	int zmin = sampP.z - lhv;
	int xmax = sampP.x + rhv - 1;
	int ymax = sampP.y + rhv - 1;
	int zmax = sampP.z + rhv - 1;

	for (int i = xmin; i <= xmax; ++i)
		for (int j = ymin; j <= ymax; ++j)
			for (int k = zmin; k <= zmax; ++k)
			{
				int binIdx = getValueBin(ivec3(i, j, k));
				hist[binIdx]++;
			}

	float num = pow(float(S), 3.0f);

	float entropy = 0.0;
	for (int i = 0; i < NumIntervals; ++i)
	{
		if (hist[i] == 0)	continue;

		float v = float(hist[i]) / num;
		entropy += (-v * log(v));
	}
	imageStore(tex_VolumeEntropy, sampP, vec4(entropy));
}