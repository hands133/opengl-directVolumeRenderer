#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)	uniform writeonly image3D tex_ReconResult;
layout(binding = 1)			uniform sampler3D tex_ReconFullVolume;

layout(binding = 2)			uniform usampler2D tex_OctreeNode;

float PI = 3.141592653589793238462643383279;

uniform ivec3 dataRes;
uniform int B;
uniform int PS;

bool IsLeafNode(uint R)
{
	return (R & 0x80000000) != 0;
}

ivec2 GetChildOffset(uint R)
{
	return ivec2((R & 0x3fff8000) >> 15, (R & 0x00007fff));
}

vec4 traverseOctree(vec3 pos)
{
	ivec2 iter = ivec2(0);
    vec4 nPosW = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	while (true)
	{
		uint R = texelFetch(tex_OctreeNode, iter, 0).x;
		if (IsLeafNode(R))	return nPosW;

        int I = 0;
        if (pos.x > nPosW.x + nPosW.w / 2.0f)   { I += 1; nPosW.x += nPosW.w / 2.0f; }
        if (pos.y > nPosW.y + nPosW.w / 2.0f)   { I += 2; nPosW.y += nPosW.w / 2.0f; }
        if (pos.z > nPosW.z + nPosW.w / 2.0f)   { I += 4; nPosW.z += nPosW.w / 2.0f; }
        nPosW.w /= 2.0f;

		iter = GetChildOffset(R) + ivec2(I, 0);
	}
}

float lerp(float a, float b, float t)
{
	return (1.0 - t) * a + t * b;
}

float lerp3(float v000, float v001, float v010, float v011,
	float v100, float v101, float v110, float v111, vec3 t)
{
	float v00 = lerp(v000, v001, t.x);
	float v01 = lerp(v010, v011, t.x);
	float v10 = lerp(v100, v101, t.x);
	float v11 = lerp(v110, v111, t.x);

	float v0 = lerp(v00, v01, t.y);
	float v1 = lerp(v10, v11, t.y);

	return lerp(v0, v1, t.z);
}

float getFromNeighboringIntegerCoord(vec3 p)
{
	vec3 gCoord = p * vec3(dataRes - ivec3(1));
	ivec3 c000 = ivec3(floor(gCoord));

	float v000 = texelFetch(tex_ReconFullVolume, c000 + ivec3(0, 0, 0), 0).x;
	float v001 = texelFetch(tex_ReconFullVolume, c000 + ivec3(1, 0, 0), 0).x;
	float v010 = texelFetch(tex_ReconFullVolume, c000 + ivec3(0, 1, 0), 0).x;
	float v011 = texelFetch(tex_ReconFullVolume, c000 + ivec3(1, 1, 0), 0).x;
	float v100 = texelFetch(tex_ReconFullVolume, c000 + ivec3(0, 0, 1), 0).x;
	float v101 = texelFetch(tex_ReconFullVolume, c000 + ivec3(1, 0, 1), 0).x;
	float v110 = texelFetch(tex_ReconFullVolume, c000 + ivec3(0, 1, 1), 0).x;
	float v111 = texelFetch(tex_ReconFullVolume, c000 + ivec3(1, 1, 1), 0).x;

	vec3 t = gCoord - c000;
	return lerp3(v000, v001, v010, v011, v100, v101, v110, v111, t);
}

void main()
{
	ivec3 SPos = ivec3(gl_GlobalInvocationID.xyz);
	vec3 SCoord = vec3(SPos) / vec3(dataRes - ivec3(1));

	vec4 leafInfo = traverseOctree(SCoord);

	float cellW = leafInfo.w / float(PS);
	vec3 o = leafInfo.xyz + cellW * floor((SCoord - leafInfo.xyz) / vec3(cellW));

	float v000 = getFromNeighboringIntegerCoord(o + vec3(0, 0, 0) * vec3(cellW));
	float v001 = getFromNeighboringIntegerCoord(o + vec3(1, 0, 0) * vec3(cellW));
	float v010 = getFromNeighboringIntegerCoord(o + vec3(0, 1, 0) * vec3(cellW));
	float v011 = getFromNeighboringIntegerCoord(o + vec3(1, 1, 0) * vec3(cellW));
	float v100 = getFromNeighboringIntegerCoord(o + vec3(0, 0, 1) * vec3(cellW));
	float v101 = getFromNeighboringIntegerCoord(o + vec3(1, 0, 1) * vec3(cellW));
	float v110 = getFromNeighboringIntegerCoord(o + vec3(0, 1, 1) * vec3(cellW));
	float v111 = getFromNeighboringIntegerCoord(o + vec3(1, 1, 1) * vec3(cellW));
	
	vec3 t = (SCoord - o) / vec3(cellW);
	float m = lerp3(v000, v001, v010, v011, v100, v101, v110, v111, t);

	imageStore(tex_ReconResult, SPos, vec4(m));
}