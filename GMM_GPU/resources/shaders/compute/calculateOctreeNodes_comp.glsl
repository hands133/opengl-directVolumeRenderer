#version 460 core

layout(local_size_x = 1) in;

layout(binding = 0, r32ui)	uniform coherent uimage2D tex_OctreeNode;

uniform int L;
uniform int texW;
uniform int PS;

bool IsLeafNode(uint R)
{
	return (R & 0x80000000) != 0;
}

void main()
{
	ivec2 pBase = ivec2(0, 0);
	if (L <= 5)	pBase.y = L;
	else
	{
		pBase.y = 5;
		for (int i = 0; i < L - 5; ++i)
			pBase.y += int(pow(8, i));
	}

	int seqL = int(gl_GlobalInvocationID.x);
	ivec2 pPos = pBase + ivec2(seqL % texW, seqL / texW);

	uint R = imageLoad(tex_OctreeNode, pPos).x;

	if (R == 0) {}
	else if (IsLeafNode(R))	imageAtomicAdd(tex_OctreeNode, ivec2(1, 0), 1);
	else					imageAtomicAdd(tex_OctreeNode, ivec2(2, 0), 1);
}