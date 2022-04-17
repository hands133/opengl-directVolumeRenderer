#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)		uniform coherent image3D tex_Volume;

uniform ivec3 UpRes;	// Upper-level resolution

bool Valid = true;
float GetValue(ivec3 P)
{
	float v = 0.0f;

	if (P.x >= UpRes.x || P.y >= UpRes.y || P.z >= UpRes.z)	Valid = false;
	if (Valid)	v = imageLoad(tex_Volume, P).x;

	return v;
}

void main()
{
	ivec3 bP = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 P = bP * ivec3(2);

	float s = 0.0f;
	int count = 0;

	float v1 = GetValue(P + ivec3(0, 0, 0));
	if (Valid) { s += v1; count++; }

	float v2 = GetValue(P + ivec3(0, 0, 1));
	if (Valid) { s += v2; count++; }

	float v3 = GetValue(P + ivec3(0, 1, 0));
	if (Valid) { s += v3; count++; }

	float v4 = GetValue(P + ivec3(0, 1, 1));
	if (Valid) { s += v4; count++; }

	float v5 = GetValue(P + ivec3(1, 0, 0));
	if (Valid) { s += v5; count++; }

	float v6 = GetValue(P + ivec3(1, 0, 1));
	if (Valid) { s += v6; count++; }

	float v7 = GetValue(P + ivec3(1, 1, 0));
	if (Valid) { s += v7; count++; }

	float v8 = GetValue(P + ivec3(1, 1, 1));
	if (Valid) { s += v8; count++; }

	if (count > 0)	s = s / float(count);

	barrier();

	imageStore(tex_Volume, bP, vec4(s));
}