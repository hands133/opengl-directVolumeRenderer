#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform coherent image2D tex_RMSENSSIM;

layout(binding = 1)			uniform sampler3D tex_Volume1;
layout(binding = 2)			uniform sampler3D tex_Volume2;

uniform int mipmapLVL;
uniform ivec3 R;

void main()
{
	ivec3 P = ivec3(gl_GlobalInvocationID.xyz);

	ivec2 mxIter = ivec2(0, mipmapLVL + 1);
	ivec2 myIter = ivec2(1, mipmapLVL + 1);

	float vx = texelFetch(tex_Volume1, P, 0).x;
	float vy = texelFetch(tex_Volume2, P, 0).x;

	imageAtomicAdd(tex_RMSENSSIM, mxIter, vx);
	imageAtomicAdd(tex_RMSENSSIM, myIter, vy);
}