#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform writeonly image3D tex_VolumeEntropy;
layout(binding = 1)			uniform sampler3D tex_Volume;

uniform ivec3 res;

uniform float vMin;
uniform float vMax;
uniform int NumIntervals;
uniform int StencilSize;

float localEntropyLocalHist(ivec3 p, int S, ivec3 res)
{
	float hist[256];
	for (int i = 0; i < NumIntervals; ++i)	hist[i] = 0.0f;

	int lhv = S / 2;
	int rhv = S / 2 + S % 2;

	int xmin = p.x - lhv;
	int ymin = p.y - lhv;
	int zmin = p.z - lhv;
	int xmax = p.x + rhv - 1;
	int ymax = p.y + rhv - 1;
	int zmax = p.z + rhv - 1;

	int r = 0;
	int s = 0;
	int t = 0;

	float dv = (vMax - vMin) / float(NumIntervals);

	for (int i = xmin; i <= xmax; ++i)
		for (int j = ymin; j <= ymax; ++j)
			for (int k = zmin; k <= zmax; ++k)
			{
				r = i;
				s = j;
				t = k;

				if (i < 0)			r = -i;
				if (j < 0)			s = -j;
				if (k < 0)			t = -k;
				if (i > res.x - 1)	r = 2 * (res.x - 1) - i;
				if (j > res.y - 1)	s = 2 * (res.y - 1) - j;
				if (k > res.z - 1)	t = 2 * (res.z - 1) - k;

				float v = texelFetch(tex_Volume, ivec3(r, s, t), 0).x;
				int vidx = int(floor((v - vMin) / dv));

				hist[vidx] += 1.0f;
			}

	float entropy = 0.0f;
	float num = pow(float(S), 3.0f);

	for (int i = 0; i < NumIntervals; ++i)
	{
		float v = hist[i] / num;
		if (v > 0)		entropy += (-v * log(v));
	}

	return entropy;
}

void main()
{
	ivec3 volRes = ivec3(gl_NumWorkGroups.xyz);
	ivec3 sampP = ivec3(gl_GlobalInvocationID.xyz);

	float e = localEntropyLocalHist(sampP, StencilSize, volRes);
	imageStore(tex_VolumeEntropy, sampP, vec4(e));
}