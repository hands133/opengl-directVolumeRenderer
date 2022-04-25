#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0)			uniform sampler3D tex_Volume;
layout(binding = 1, r32ui)	uniform coherent uimage2D tex_Histogram;

uniform float vMin;
uniform float vMax;

uniform int NumIntervals;

void main()
{
	ivec3 sampP = ivec3(gl_GlobalInvocationID.xyz);

	float dv = (vMax - vMin) / float(NumIntervals);
	float v = texelFetch(tex_Volume, sampP, 0).x;
	int bIdx = int((v - vMin) / dv);

	imageAtomicAdd(tex_Histogram, ivec2(bIdx, 0), 1);
}