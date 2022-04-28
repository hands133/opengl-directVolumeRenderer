#version 460 core

layout(local_size_x = 1, local_size_y = 1) in;

layout(binding = 0, r32ui)		uniform coherent uimage2D tex_OctreeNode;
layout(binding = 1, rgba32f)	uniform coherent image2D tex_OctreePos;


uniform int currentLevel;
uniform int texW;
uniform int PS;		// patch-size

bool Flagged(uint R);
uint SetInnerNode(uint R);
ivec2 GetChildOffset(uint R);
vec3 BlockSeq2Coord(uint S, vec4 C);

void main()
{
	int L = currentLevel;

	// calculate O
	ivec2 pBase = ivec2(0, 0);
	if (L <= 5)	pBase.y = L;
	else {
		pBase.y = 5;
		for (int i = 0; i < L - 5; ++i)
			pBase.y += int(pow(8, i));
	}

	// calculate Pos
	int seqL = int(gl_GlobalInvocationID.x);
	ivec2 pPos = pBase + ivec2(seqL % texW, seqL / texW);

	// Check if flagged
	uint Node = imageLoad(tex_OctreeNode, pPos).x;
	if (Node == 0)			return;
	if (!Flagged(Node))		return;		// not flagged

	if (gl_GlobalInvocationID.y == 0)
		imageStore(tex_OctreeNode, pPos, uvec4(SetInnerNode(Node)));

	// Child Iter
	uint childSeq = gl_GlobalInvocationID.y;

	ivec2 pChild = GetChildOffset(Node) + ivec2(childSeq, 0);
	imageStore(tex_OctreeNode, pChild, uvec4(1 << 31));
}

bool Flagged(uint R)
{
	return (R & (1 << 30)) != 0;
}

uint SetInnerNode(uint R)
{
	return R & 0x7fffffff;
}

ivec2 GetChildOffset(uint R)
{
	return ivec2((R & 0x3fff8000) >> 15, (R & 0x00007fff));
}

vec3 BlockSeq2Coord(uint S, vec4 C)
{
	uint S0 = S;
	uint Z = S0 / 4;
	S0 -= Z * 4;
	uint Y = S0 / 2;
	S0 -= Y * 2;
	uint X = S0;

	return vec3(X, Y, Z) * vec3(C.w / 2.0f) + C.xyz;
}