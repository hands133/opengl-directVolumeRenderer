#version 460 core

#extension GL_NV_shader_atomic_float : require

layout(local_size_x = 1) in;

layout(binding = 0, r32ui)		uniform coherent uimage2D tex_OctreeNode;
layout(binding = 1)				uniform sampler3D tex_Entropy;

uniform int currentLevel;
uniform int texW;
uniform int PS;		// patch-size
uniform float eValve;

uint SetFlag(uint R);
bool JudgeFlag(vec4 cellInfo, float e);
uint SetChildOffset(uint R, ivec2 P);

ivec3 invZOrder(int lvl, int seqIdx)
{
	ivec3 idx = ivec3(0);
	for (int i = 0; i < lvl; ++i)
	{
		idx.x |= ((seqIdx & (1 << (0 + 3 * i))) >> (2 * i + 0));
		idx.y |= ((seqIdx & (1 << (1 + 3 * i))) >> (2 * i + 1));
		idx.z |= ((seqIdx & (1 << (2 + 3 * i))) >> (2 * i + 2));
	}
	return idx;
}

vec4 getNodeInfo(int lvl, int seqIdx)
{
	float cellW = 1.0f / pow(2.0, float(lvl));
	ivec3 coord = invZOrder(lvl, seqIdx);
	return vec4(vec3(coord) * vec3(cellW), cellW);
}

void main()
{
	int L = currentLevel;
	if (L == 0)	imageStore(tex_OctreeNode, ivec2(0), uvec4(1 << 31));
	 
	// calculate O
	ivec2 pBase = ivec2(0, 0);
	if (L <= 5)	pBase.y = L;
	else {
		pBase.y = 5;
		for (int i = 0; i < L - 5; ++i)	pBase.y += int(pow(8, i));
	}

	// calculate Pos
	int seqL = int(gl_GlobalInvocationID.x);
	ivec2 pPos = pBase + ivec2(seqL % texW, seqL / texW);

	// check if current node is empty
  	uint Node = imageLoad(tex_OctreeNode, pPos).x;
	if (Node == 0)	return;

	// child offset
	ivec2 pNextBase = pBase;
	if (L <= 5)		pNextBase.y++;
	else			pNextBase.y += int(pow(8, L - 5));

	uint nNode = (pPos.y - pBase.y) * texW + pPos.x;
	uint nChild = nNode * 8;
	ivec2 pChildOffset = pNextBase + ivec2(nChild % texW, nChild / texW);

	vec4 cellInfo = getNodeInfo(L, seqL);
	if (JudgeFlag(cellInfo, eValve))
	{
		Node = SetFlag(Node);
		Node = SetChildOffset(Node, pChildOffset);
		imageStore(tex_OctreeNode, pPos, uvec4(Node));
	};
}

uint SetFlag(uint R)
{
	return R | (1 << 30);
}

bool JudgeFlag(vec4 cellInfo, float e)
{
	float w = cellInfo.w;
	float w0 = w / float(PS);
	float wOffset = w0 / 2.0f;

	float vMin = 100.0f;
	float vMax = -1.0f;
	bool shouldDivide = false;
	
	for (int i = 0; i < PS + 1; ++i)
		for (int j = 0; j < PS + 1; ++j)
			for (int k = 0; k < PS + 1; ++k)
			{
				vec3 pSamp = cellInfo.xyz + vec3(w0) * vec3(i, j, k);
				float entropy = texture(tex_Entropy, pSamp).x;
				if (entropy >= e)	shouldDivide = true;
			}
	return shouldDivide;
}

uint SetChildOffset(uint R, ivec2 P)
{
	return R | (P.x << 15) | P.y;
}
