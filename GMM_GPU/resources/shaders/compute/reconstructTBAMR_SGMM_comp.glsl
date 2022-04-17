#version 460 core
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f)   uniform writeonly image3D tex_ReconResult;

layout(binding = 1)         uniform usampler2D tex_OctreeNode;
layout(binding = 2)         uniform sampler2D tex_OctreePos;

layout(binding = 3)			uniform sampler3D tex_GMMCoeff_1;
layout(binding = 4)			uniform sampler3D tex_GMMCoeff_2;
layout(binding = 5)			uniform sampler3D tex_GMMCoeff_3;
layout(binding = 6)			uniform sampler3D tex_GMMCoeff_4;

float PI = 3.141592653589793238462643383279;

uniform int B;
uniform int PS;

uniform int NumIntervals;
uniform int NumBricks;

uniform ivec3 BrickRes;
uniform ivec3 DataRes;

uniform int XGap[10];
uniform int YGap[10];
uniform int ZGap[10];

uniform int XBlockGap[10];
uniform int YBlockGap[10];
uniform int ZBlockGap[10];

uniform ivec3 O;
uniform ivec3 R;
uniform int BRICK_IDX;

shared vec4 leafInfo;
shared float probBuffer[256];

// GMM-core
float gaussian1D(float mean, float var2, float x);
float gaussian3D(vec3 means, vec3 var2s, vec3 p);

// GMM-evaluate value at p
float evaluateProb(vec3 p, ivec3 texCoord);
vec2 evaluateValuePerTex(ivec3 texCoord, vec3 p, sampler3D GMMCoefftex);

// TB-AMR method
bool IsLeafNode(uint R);
ivec2 GetChildOffset(uint R);
vec4 traverseOctree(vec3 pos);

// Utility Method
float lerp(float a, float b, float t);
float lerp3(float v000, float v001, float v010, float v011,
	float v100, float v101, float v110, float v111, vec3 t);

// Utility Method of Shared Variable
float calProbAtEachBin(vec3 c, int binIdx);
int calBinIdxWithMaxProb();


void main()
{
    ivec3 sp = ivec3(gl_GlobalInvocationID.xyz) / ivec3(NumIntervals, 1, 1);
    ivec3 samplePoint = sp + ivec3(O);
    vec3 sc = vec3(samplePoint) / vec3(DataRes - ivec3(1));

    int binIdx = int(gl_LocalInvocationID.x);   // [0 ~ NumIntervals)

    // 1. traverse octree, find the leaf node and calculate vertices
    if (binIdx == 0)    leafInfo = traverseOctree(sc);
    barrier();

    float leafW = leafInfo.w;
    // 2. calculate patch size and the pach which sp locates
    float patchW = leafW / float(PS);
    ivec3 patchCellPoint = ivec3((sc - leafInfo.xyz) / vec3(patchW));
	vec3 o = leafInfo.xyz + patchW * vec3(patchCellPoint);

    vec3 c000 = o + vec3(0, 0, 0) * vec3(patchW);
    vec3 c001 = o + vec3(1, 0, 0) * vec3(patchW);
    vec3 c010 = o + vec3(0, 1, 0) * vec3(patchW);
    vec3 c011 = o + vec3(1, 1, 0) * vec3(patchW);
    vec3 c100 = o + vec3(0, 0, 1) * vec3(patchW);
    vec3 c101 = o + vec3(1, 0, 1) * vec3(patchW);
    vec3 c110 = o + vec3(0, 1, 1) * vec3(patchW);
    vec3 c111 = o + vec3(1, 1, 1) * vec3(patchW);

    vec3 t = (sc - o) / vec3(patchW);

    // 3. calculate vijk one-by-one
    // probBuffer[binIdx] = calProbAtEachBin(c000, binIdx);
    // barrier();
    // float v000 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c001, binIdx);
    // barrier();
    // float v001 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c010, binIdx);
    // barrier();
    // float v010 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c011, binIdx);
    // barrier();
    // float v011 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c100, binIdx);
    // barrier();
    // float v100 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c101, binIdx);
    // barrier();
    // float v101 = float(calBinIdxWithMaxProb());

    // probBuffer[binIdx] = calProbAtEachBin(c110, binIdx);
    // barrier();
    // float v110 = float(calBinIdxWithMaxProb());
    
    // probBuffer[binIdx] = calProbAtEachBin(c111, binIdx);
    // barrier();
    // float v111 = float(calBinIdxWithMaxProb());

    // float m = lerp3(v000, v001, v010, v011, v100, v101, v110, v111, t);

    probBuffer[binIdx] = calProbAtEachBin(o + vec3(0.5f) * vec3(patchW), binIdx);
    barrier();
    
    if (binIdx == 0)
    {
        float m = float(calBinIdxWithMaxProb());
        imageStore(tex_ReconResult, samplePoint, vec4(m));
    }
}

// Function Implementations
float gaussian1D(float mean, float var2, float x)
{
	float v = 1.0f / sqrt(2.0f * PI * var2);
	return v * exp(-0.5f * (x - mean) * (x - mean) / var2);
}

float gaussian3D(vec3 means, vec3 var2s, vec3 p)
{
	float vx = gaussian1D(means.x, var2s.x, p.x);
	float vy = gaussian1D(means.y, var2s.y, p.y);
	float vz = gaussian1D(means.z, var2s.z, p.z);
	return vx * vy * vz;
}

vec2 evaluateValuePerTex(ivec3 texCoord, vec3 p, sampler3D GMMCoefftex)
{
	ivec3 coordMean = texCoord;
	ivec3 coordVar2 = texCoord + ivec3(0, 0, NumBricks);

	vec4 means = texelFetch(GMMCoefftex, coordMean, 0);
	vec4 var2s = texelFetch(GMMCoefftex, coordVar2, 0);

	float binWeight = means.w;
	float kernelValue = var2s.w * gaussian3D(means.xyz, var2s.xyz, p);

	return vec2(binWeight, kernelValue);
}

float evaluateProb(vec3 p, ivec3 texCoord)
{
	vec2 V1 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_1);
	vec2 V2 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_2);
	vec2 V3 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_3);
	vec2 V4 = evaluateValuePerTex(texCoord, p, tex_GMMCoeff_4);

	return V1.x * (V1.y + V2.y + V3.y + V4.y);
}

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
	vec4 nPosW = vec4(0.0f);

	while (true)
	{
		nPosW = texelFetch(tex_OctreePos, iter, 0);
		uint R = texelFetch(tex_OctreeNode, iter, 0).x;

		if (IsLeafNode(R))	return nPosW;

		int I = 0;
		if (pos.x > nPosW.x + nPosW.w / 2.0f)  I += 1;
		if (pos.y > nPosW.y + nPosW.w / 2.0f)  I += 2;
		if (pos.z > nPosW.z + nPosW.w / 2.0f)  I += 4;

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

float calProbAtEachBin(vec3 c, int binIdx)
{
    // 0. base point
    vec3 sc = c * vec3(DataRes - ivec3(1));
    ivec3 gp = ivec3(sc);
    
    // 1. find which brick the p locates
    ivec3 brickIdx;
    for (int i = 0; i < BrickRes.x; ++i)
        for (int j = 0; j < BrickRes.y; ++j)
            for (int k = 0; k < BrickRes.z; ++k) {
                if (gp.x >= XGap[i] && gp.x < XGap[i + 1])   brickIdx.x = i;    
                if (gp.y >= YGap[j] && gp.y < YGap[j + 1])   brickIdx.y = j;    
                if (gp.z >= ZGap[k] && gp.z < ZGap[k + 1])   brickIdx.z = k;    
            }
    
    int brickSeqIdx = brickIdx.z * BrickRes.y * BrickRes.x + brickIdx.y * BrickRes.x + brickIdx.x;

    ivec3 brickO = ivec3(XGap[brickIdx.x], YGap[brickIdx.y], ZGap[brickIdx.z]);
    ivec3 brickR = ivec3(XGap[brickIdx.x + 1], YGap[brickIdx.y + 1], ZGap[brickIdx.z + 1]) - brickO;

    ivec3 brickBlockO = ivec3(XBlockGap[brickIdx.x], YBlockGap[brickIdx.y], ZBlockGap[brickIdx.z]);
    ivec3 brickBlockR = ivec3(XBlockGap[brickIdx.x + 1],
                            YBlockGap[brickIdx.y + 1],
                            ZBlockGap[brickIdx.z + 1]) - brickBlockO;

    uvec3 brickOffsetPos = gp - brickO;
	uvec3 sampP = brickOffsetPos % uvec3(B);

	uvec3 brickBlockRes = brickR / uvec3(B);
	uvec3 brickBlockPos = brickOffsetPos / uvec3(B);

	if (brickOffsetPos.x >= brickBlockRes.x * B)
	{
		sampP.x += B;
		brickBlockPos.x--;
	}

	if (brickOffsetPos.y >= brickBlockRes.y * B)
	{
		sampP.y += B;
		brickBlockPos.y--;
	}

	if (brickOffsetPos.z >= brickBlockRes.z * B)
	{
		sampP.z += B;
		brickBlockPos.z--;
	}

	uint blockIdx = brickBlockPos.z * brickBlockRes.y * brickBlockRes.x +
		brickBlockPos.y * brickBlockRes.x + brickBlockPos.x;

	// step 3. calculate texture coordiantes
	ivec3 texCoord = ivec3(blockIdx, binIdx, brickSeqIdx);
    return evaluateProb(sampP, texCoord);
}

int calBinIdxWithMaxProb()
{
    float prob = -1.0f;
    int binIdx = 0;
    for (int i = 0; i < NumIntervals; ++i)
    {
        if (probBuffer[i] > prob)
        {
            prob = probBuffer[i];
            binIdx = i;
        }
    }
    return binIdx;
}
