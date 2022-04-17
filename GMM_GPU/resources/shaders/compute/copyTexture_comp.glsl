#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform writeonly image3D tex_Target;
layout(binding = 1)			uniform sampler3D tex_Source;

void main()
{
	ivec3 P = ivec3(gl_GlobalInvocationID.xyz);

	float v = texelFetch(tex_Source, P, 0).x;
	imageStore(tex_Target, P, vec4(v));
}