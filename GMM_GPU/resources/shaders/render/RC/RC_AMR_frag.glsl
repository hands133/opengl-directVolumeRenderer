#version 460 core

out vec4 FragColor;

uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform usampler2D octreeNodePool;
uniform sampler2D octreeNodePos;

uniform sampler2D coordIn;
uniform sampler2D coordOut;

uniform usampler3D volumeTex;
uniform sampler3D entropyTex;
uniform bool showVolOrTex;
uniform sampler1D tFunc;

uniform float vMin;
uniform float vMax;
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

int LVL = -1;
bool xMost = false;
bool yMost = false;
bool zMost = false;

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

    for (int i = 0; i < numSamp; ++i)
    {
        vec4 tmpColor = vec4(0.0f);
        if (showGrid)   tmpColor = GetColor(currentPos);
        else            tmpColor = texture(tFunc, (traverseOctree(currentPos) - vMin) / (vMax - vMin));


        // draw clip
        if (clipVolume)
        {
            //if (currentPos.z < clipCoord_x) tmpColor.a = 0.0f;
            if (currentPos.x > clipCoord_x) tmpColor.a = 0.0f;
            else                            tmpColor.a = 1.0f;
        }

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
    vec4 nPosW = vec4(0.0f);

    LVL = -1;
    int tmpLVL = 0;
    while (true)
    {
        nPosW = texelFetch(octreeNodePos, iter, 0);
        if (LVL == -1) {
            float borderWidth = (1.0f / 300.0f) * pow(0.8, float(tmpLVL));
            vec3 dis = pos - nPosW.xyz;

            xMost = min(abs(dis.x), abs(dis.x - nPosW.w)) < borderWidth;
            yMost = min(abs(dis.y), abs(dis.y - nPosW.w)) < borderWidth;
            zMost = min(abs(dis.z), abs(dis.z - nPosW.w)) < borderWidth;

            bool showGridCond = false;
            //if (clipVolume)     showGridCond = xMost || yMost || (xMost && zMost) || (yMost && zMost);
            if (clipVolume)     showGridCond = yMost || zMost || (xMost && yMost) || (xMost && zMost);
            else                showGridCond = (xMost && yMost) || (xMost && zMost) || (yMost && zMost);
            if (showGridCond)   LVL = tmpLVL;
        }

        int I = 0;
        if (pos.x > nPosW.x + nPosW.w / 2.0f)  I += 1;
        if (pos.y > nPosW.y + nPosW.w / 2.0f)  I += 2;
        if (pos.z > nPosW.z + nPosW.w / 2.0f)  I += 4;

        uint R = texelFetch(octreeNodePool, iter, 0).x;
        if (IsLeafNode(R))  break;

        iter = GetChildOffset(R) + ivec2(I, 0);
        tmpLVL++;
        if (tmpLVL > maxSearchDepth) break;
    }
    return nPosW;
}

float InterpInCell(vec4 nPosW, float PS, vec3 pos)
{
    vec3 dis = pos - nPosW.xyz;
    float subBlockSize = nPosW.w / float(patchSize);
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
        uint b0 = texture(volumeTex, node0).x;
        uint b1 = texture(volumeTex, node1).x;
        uint b2 = texture(volumeTex, node2).x;
        uint b3 = texture(volumeTex, node3).x;
        uint b4 = texture(volumeTex, node4).x;
        uint b5 = texture(volumeTex, node5).x;
        uint b6 = texture(volumeTex, node6).x;
        uint b7 = texture(volumeTex, node7).x;

        v0 = vMin + float(b0) * dv;
        v1 = vMin + float(b1) * dv;
        v2 = vMin + float(b2) * dv;
        v3 = vMin + float(b3) * dv;
        v4 = vMin + float(b4) * dv;
        v5 = vMin + float(b5) * dv;
        v6 = vMin + float(b6) * dv;
        v7 = vMin + float(b7) * dv;
    }
    else
    {
        v0 = texture(entropyTex, node0).x;
        v1 = texture(entropyTex, node1).x;
        v2 = texture(entropyTex, node2).x;
        v3 = texture(entropyTex, node3).x;
        v4 = texture(entropyTex, node4).x;
        v5 = texture(entropyTex, node5).x;
        v6 = texture(entropyTex, node6).x;
        v7 = texture(entropyTex, node7).x;
    }

    vec3 vd = (pos - node0) / vec3(subBlockSize);
    float v = v0 * (1.0 - vd.x) * (1.0 - vd.y) * (1.0 - vd.z)
        + v1 * vd.x * (1.0 - vd.y) * (1.0 - vd.z)
        + v2 * (1.0 - vd.x) * vd.y * (1.0 - vd.z)
        + v3 * vd.x * vd.y * (1.0 - vd.z)
        + v4 * (1.0 - vd.x) * (1.0 - vd.y) * vd.z
        + v5 * vd.x * (1.0 - vd.y) * vd.z
        + v6 * (1.0 - vd.x) * vd.y * vd.z
        + v7 * vd.x * vd.y * vd.z;

    return v;
}

float traverseOctree(vec3 pos)
{
    vec4 nPosW = traverseOctree(pos, UserDefinedDepth);
    return InterpInCell(nPosW, patchSize, pos);
}

vec4 GetColor(vec3 pos)
{
    vec4 nPosW = traverseOctree(pos, UserDefinedDepth);

    if (LVL >= 0)   return GColor;

    if (showPatch && patchSize > 1)
    {
        float subBlockSize = nPosW.w / float(patchSize);
        vec3 dis = pos - nPosW.xyz;
        vec3 idx = floor(dis / vec3(subBlockSize));

        vec3 subDis = pos - (nPosW.xyz + idx * vec3(subBlockSize));
        float subBorderWidth = 1.0f / 750.0f * pow(0.9f, float(LVL));

        bool subxMost = min(abs(subDis.x), abs(subDis.x - nPosW.w)) < subBorderWidth;
        bool subyMost = min(abs(subDis.y), abs(subDis.y - nPosW.w)) < subBorderWidth;
        bool subzMost = min(abs(subDis.z), abs(subDis.z - nPosW.w)) < subBorderWidth;

        bool showGridCond = false;

        //if (clipVolume) showGridCond = subxMost || subyMost || (subxMost && subzMost) || (subyMost && subzMost);
        if (clipVolume) showGridCond = subyMost || subzMost || (subxMost && subyMost) || (subxMost && subzMost);
        else            showGridCond = (subxMost && subyMost) || (subxMost && subzMost) || (subyMost && subzMost);
        if (showGridCond)   return GColor;
    }

    float v = InterpInCell(nPosW, patchSize, pos);
    vec4 tmpColor = texture(tFunc, (v - vMin) / (vMax - vMin), 0.0f);
    tmpColor.a = 1.0 - pow(1.0 - tmpColor.a, GAMMA);
    
    return tmpColor;
}