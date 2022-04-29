#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32ui)	uniform coherent uimage2D tex_MinMaxValue;
layout(binding = 1)			uniform sampler3D tex_Entropy;

void main()
{
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);
	float v = texelFetch(tex_Entropy, p, 0).x;

	imageAtomicMin(tex_MinMaxValue, ivec2(0, 0), int(v * 1.0e8));
	imageAtomicMax(tex_MinMaxValue, ivec2(1, 0), int(v * 1.0e8));
}