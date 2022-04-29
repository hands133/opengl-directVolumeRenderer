#version 460 core

out vec4 FragColor;

uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform usampler2D octreeNodePool;

uniform sampler2D coordIn;
uniform sampler2D coordOut;

uniform usampler3D volumeTexU8;
uniform sampler3D entropyTexF32;
uniform bool showVolOrTex;
uniform sampler1D tFunc;

uniform float vMin;
uniform float vMax;
uniform ivec3 dataRes;
uniform int NumIntervals;

uniform int maxOctreeDepth;
uniform int patchSize;
uniform int UserDefinedDepth;

uniform bool showGrid;
uniform bool showPatch;
uniform bool clipVolume;
uniform float clipPlane;

uniform float GAMMA;
uniform vec4 GColor;    // Grid-line color

float traverseOctree(vec3 pos);
vec4 GetColor(vec3 pos);


float lerp(float a, float b, float t);
float lerp3(float v000, float v001, float v010, float v011,
	float v100, float v101, float v110, float v111, vec3 t);

int LVL = -1;
bool xMost = false;
bool yMost = false;
bool zMost = false;

bool isInInverval(float l, float r, float v)
{
    float m = min(l, r);
    float M = max(l, r);
    return (v >= m && v <= M);
}

void main()
{
    vec2 coordOnScreen = vec2(gl_FragCoord.x + 0.5, gl_FragCoord.y + 0.5);
    coordOnScreen /= vec2(SCR_WIDTH - 1, SCR_HEIGHT - 1);
    vec3 cdIn = texture(coordIn, coordOnScreen).xyz;
    vec3 cdOut = texture(coordOut, coordOnScreen).xyz;

    if (cdIn == cdOut)  discard;

    float stepSize = 0.001f;
    vec3 dir = cdOut - cdIn;

    vec3 currentPos = cdIn;
    vec3 color = vec3(0.0);
    float alpha = 0.0;

    vec3 deltaDir = dir * (stepSize / length(dir));
    float numSamp = length(dir) / stepSize;
    float clipCoord_x = clipPlane;

    if (clipVolume)
    {
        float t = (clipCoord_x - cdIn.x) / (cdOut.x - cdIn.x);
        currentPos.x = clipCoord_x;
        currentPos.y = cdIn.y + t * (cdOut.y - cdIn.y);
        currentPos.z = cdIn.z + t * (cdOut.z - cdIn.z);
        
        vec4 tmpColor = vec4(0.0f);
        if (showGrid)   tmpColor = GetColor(currentPos);
        else            tmpColor = texture(tFunc, (traverseOctree(currentPos) - vMin) / (vMax - vMin));
        color = tmpColor.xyz;
        alpha = isInInverval(cdIn.x, cdOut.x, clipCoord_x) ? 1.0f : 0.0f;
    }
    else
    {
        for (int i = 0; i < numSamp; ++i)
        {
            vec4 tmpColor = vec4(0.0f);
            if (showGrid)   tmpColor = GetColor(currentPos);
            else            tmpColor = texture(tFunc, (traverseOctree(currentPos) - vMin) / (vMax - vMin));

            if (tmpColor.a > 0.0)
            {
                if(!showGrid)   tmpColor.a = 1.0 - pow(1.0 - tmpColor.a, GAMMA);
                color += (1.0 - alpha) * tmpColor.rgb * tmpColor.a;
                alpha += (1.0 - alpha) * tmpColor.a;
            }
            currentPos += deltaDir;

            if (alpha >= 1.0)   // if grid line has trasparency
            {
                alpha = 1.0;
                break;
            }
        }
    }
    FragColor = vec4(color, alpha);
}

ivec2 GetChildOffset(uint R)
{
    return ivec2((R & 0x3fff8000) >> 15, (R & 0x00007fff));
}

bool IsLeafNode(uint R)
{
    return (R & 0x80000000) != 0;
}

vec4 traverseOctree(vec3 pos, int maxSearchDepth)
{
    ivec2 iter = ivec2(0);
    vec4 nPosW = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    LVL = -1;
    bool updateLVL = true;
    int tmpLVL = 0;

    float minAxis = 1.0e8;
    minAxis = (float(dataRes.x) < minAxis) ? float(dataRes.x) : minAxis;
    minAxis = (float(dataRes.y) < minAxis) ? float(dataRes.y) : minAxis;
    minAxis = (float(dataRes.z) < minAxis) ? float(dataRes.z) : minAxis;
    float xScale = dataRes.x / minAxis;
    float yScale = dataRes.y / minAxis;
    float zScale = dataRes.z / minAxis;
    vec3 scale = vec3(xScale, yScale, zScale);

    while (true)
    {
        if (LVL == -1) {
            float borderWidth = (1.0f / 300.0f) * pow(0.8, float(tmpLVL));
            vec3 dis = pos - nPosW.xyz;

            xMost = min(abs(dis.x), abs(dis.x - nPosW.w)) < borderWidth;
            yMost = min(abs(dis.y), abs(dis.y - nPosW.w)) < borderWidth;
            zMost = min(abs(dis.z), abs(dis.z - nPosW.w)) < borderWidth;

            bool showGridCond = false;
            // if (clipVolume)     showGridCond = xMost || yMost || (xMost && zMost) || (yMost && zMost);
            if (clipVolume)     showGridCond = yMost || zMost || (xMost && yMost) || (xMost && zMost);
            else                showGridCond = (xMost && yMost) || (xMost && zMost) || (yMost && zMost);
            if (showGridCond && updateLVL)   LVL = tmpLVL;
        }

        int I = 0;
        if (pos.x > nPosW.x + nPosW.w / 2.0f)   { I += 1; nPosW.x += nPosW.w / 2.0f; }
        if (pos.y > nPosW.y + nPosW.w / 2.0f)   { I += 2; nPosW.y += nPosW.w / 2.0f; };
        if (pos.z > nPosW.z + nPosW.w / 2.0f)   { I += 4; nPosW.z += nPosW.w / 2.0f; };
        nPosW.w /= 2.0f;

        uint R = texelFetch(octreeNodePool, iter, 0).x;
        if (IsLeafNode(R))  break;

        iter = GetChildOffset(R) + ivec2(I, 0);
        tmpLVL++;
        if (tmpLVL > maxSearchDepth) { updateLVL = false; };

    }
    return nPosW;
}

// need to lerp integer texture to float
float InterpIntegerSampler(vec3 pos)
{
    float v = 0.0f;
    if (clipVolume)
    {
        vec3 bp = pos * vec3(dataRes - ivec3(1));
        ivec3 O = ivec3(bp);
        vec3 t = bp - vec3(O);
        
	    float b0 = float(texelFetch(volumeTexU8, O + ivec3(0, 0, 0), 0).x);
	    float b1 = float(texelFetch(volumeTexU8, O + ivec3(1, 0, 0), 0).x);
	    float b2 = float(texelFetch(volumeTexU8, O + ivec3(0, 1, 0), 0).x);
	    float b3 = float(texelFetch(volumeTexU8, O + ivec3(1, 1, 0), 0).x);
	    float b4 = float(texelFetch(volumeTexU8, O + ivec3(0, 0, 1), 0).x);
	    float b5 = float(texelFetch(volumeTexU8, O + ivec3(1, 0, 1), 0).x);
	    float b6 = float(texelFetch(volumeTexU8, O + ivec3(0, 1, 1), 0).x);
	    float b7 = float(texelFetch(volumeTexU8, O + ivec3(1, 1, 1), 0).x);
        return lerp3(b0, b1, b2, b3, b4, b5, b6, b7, t.xyz);
    }
    else    return float(texture(volumeTexU8, pos).x);
    return v;
}

float InterpInCell(vec4 nPosW, float PS, vec3 pos)
{
    vec3 dis = pos - nPosW.xyz;
    float subBlockSize = nPosW.w / float(PS);
    ivec3 idx = ivec3(floor(dis / vec3(subBlockSize)));
    float dv = (vMax - vMin) / float(NumIntervals);

    int i0 = idx.x;
    int j0 = idx.y;
    int k0 = idx.z;
    int i1 = i0 + 1;
    int j1 = j0 + 1;
    int k1 = k0 + 1;

    ivec3 id0 = ivec3(i0, j0, k0);
    ivec3 id1 = ivec3(i1, j0, k0);
    ivec3 id2 = ivec3(i0, j1, k0);
    ivec3 id3 = ivec3(i1, j1, k0);
    ivec3 id4 = ivec3(i0, j0, k1);
    ivec3 id5 = ivec3(i1, j0, k1);
    ivec3 id6 = ivec3(i0, j1, k1);
    ivec3 id7 = ivec3(i1, j1, k1);

    vec3 node0 = nPosW.xyz + vec3(id0) * vec3(subBlockSize);
    vec3 node1 = nPosW.xyz + vec3(id1) * vec3(subBlockSize);
    vec3 node2 = nPosW.xyz + vec3(id2) * vec3(subBlockSize);
    vec3 node3 = nPosW.xyz + vec3(id3) * vec3(subBlockSize);
    vec3 node4 = nPosW.xyz + vec3(id4) * vec3(subBlockSize);
    vec3 node5 = nPosW.xyz + vec3(id5) * vec3(subBlockSize);
    vec3 node6 = nPosW.xyz + vec3(id6) * vec3(subBlockSize);
    vec3 node7 = nPosW.xyz + vec3(id7) * vec3(subBlockSize);

    float v0 = 0.0f;
    float v1 = 0.0f;
    float v2 = 0.0f;
    float v3 = 0.0f;
    float v4 = 0.0f;
    float v5 = 0.0f;
    float v6 = 0.0f;
    float v7 = 0.0f;

    if (showVolOrTex)
    {
        v0 = InterpIntegerSampler(node0);
        v1 = InterpIntegerSampler(node1);
        v2 = InterpIntegerSampler(node2);
        v3 = InterpIntegerSampler(node3);
        v4 = InterpIntegerSampler(node4);
        v5 = InterpIntegerSampler(node5);
        v6 = InterpIntegerSampler(node6);
        v7 = InterpIntegerSampler(node7);
    }
    else
    {
        v0 = texture(entropyTexF32, node0).x;
        v1 = texture(entropyTexF32, node1).x;
        v2 = texture(entropyTexF32, node2).x;
        v3 = texture(entropyTexF32, node3).x;
        v4 = texture(entropyTexF32, node4).x;
        v5 = texture(entropyTexF32, node5).x;
        v6 = texture(entropyTexF32, node6).x;
        v7 = texture(entropyTexF32, node7).x;
    }

    vec3 t = (pos - node0) / vec3(subBlockSize);
    float v = lerp3 (v0, v1, v2, v3, v4, v5, v6, v7, t);

    return v;
}

float traverseOctree(vec3 pos)
{
    vec4 nPosW = traverseOctree(pos, UserDefinedDepth);
    return InterpInCell(nPosW, patchSize, pos);
}

vec4 GetColor(vec3 pos)
{
    float gridClipX = 0.0f;
    float gridClipY = 0.0f;
    float gridClipZ = 0.0f;
    bool atEdge = false;

    vec4 nPosW = traverseOctree(pos, UserDefinedDepth);
    // if (LVL >= 0 && pos.y > 0.5)   return GColor;
    if (LVL >= 0)   return GColor;

    float edgeDis = 0.0f;
    if (showPatch && patchSize > 1) {
        float subBlockSize = nPosW.w / float(patchSize);
        vec3 dis = pos - nPosW.xyz;
        vec3 idx = floor(dis / vec3(subBlockSize));

        vec3 subDis = pos - (nPosW.xyz + idx * vec3(subBlockSize));
        float subBorderWidth = 1.0f / 750.0f * pow(0.9f, float(LVL));

        float minDisX = min(abs(subDis.x), abs(subDis.x - nPosW.w));
        float minDisY = min(abs(subDis.y), abs(subDis.y - nPosW.w));
        float minDisZ = min(abs(subDis.z), abs(subDis.z - nPosW.w));

        bool subxMost = minDisX < subBorderWidth;
        bool subyMost = minDisY < subBorderWidth;
        bool subzMost = minDisZ < subBorderWidth;

        bool showGridCond = false;

        if (clipVolume) showGridCond = subxMost || subyMost || (subxMost && subzMost) || (subyMost && subzMost);
        //if (clipVolume) showGridCond = subyMost || subzMost || (subxMost && subyMost) || (subxMost && subzMost);
        else            showGridCond = (subxMost && subyMost) || (subxMost && subzMost) || (subyMost && subzMost);
        // if (showGridCond && pos.y > 0.5)   return GColor;
        if (showGridCond)   return GColor;
    }
    float v = InterpInCell(nPosW, patchSize, pos);
    vec4 tmpColor = texture(tFunc, (v - vMin) / (vMax - vMin), 0.0f);
    tmpColor.a = 1.0 - pow(1.0 - tmpColor.a, GAMMA);

    return tmpColor;
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