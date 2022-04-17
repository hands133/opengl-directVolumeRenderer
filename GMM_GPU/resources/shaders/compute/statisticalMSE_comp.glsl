#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform coherent image2D tex_RMSE;

layout(binding = 1)			uniform sampler3D tex_ReconFullVolume;
layout(binding = 2)			uniform sampler3D tex_ReconAMRVolume;


void main()
{
	ivec3 p = ivec3(gl_GlobalInvocationID.xyz);

	float vf = texelFetch(tex_ReconFullVolume, p, 0).x;
	float vo = texelFetch(tex_ReconAMRVolume, p, 0).x;

	float v = abs(vf - vo);

	imageAtomicAdd(tex_RMSE, ivec2(0, 0), v * v);
}