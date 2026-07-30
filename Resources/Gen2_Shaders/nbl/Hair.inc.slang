#pragma once

#include "../lib/Camera.slang"
#include "../lib/Emitter.slang"
#include "../lib/Marschner.slang"
#include "../lib/KajiyaKay.slang"

// Workgroup config
static const uint nbl_Hair_Workgroup_Size    = 32;
static const uint nbl_Hair_Max_StrandletSize = nbl_Hair_Workgroup_Size;

// Mesh Shader Config
static const uint nbl_Hair_Mesh_MaxVertices = 64;
static const uint nbl_Hair_Mesh_MaxPrimitives = 64;

struct TaskPayload
{
    // Global strand index of the first strand in the [Task] workgroup.
    uint baseID;

    // Exclusive prefix sum of strandlet counts within this [Task] workgroup.
    uint8_t deltaID[nbl_Hair_Workgroup_Size];
};

struct MeshVertex
{
    float4 position : SV_Position;
    float3 worldPosition;
    float3 worldTangent;
    float3 color;
};

struct HairVertex
{
    float3 position;
};

struct HairAttributes
{
    float3 color;
    float thickness;
    float transparency;
};

struct HairStrandDesc
{
    int strandId;
    uint vertexCount;
    uint strandletCount;
    uint firstVertex;
};

struct ScreenSpaceTriangle
{
    float3 v0;
    float3 v1;
    float3 v2;
    float3 tangent;
    float3 color;
    uint   primitiveId;
};

struct MeshPushConstants
{
    float4x4             model;
    float4               diffuse;
    float4               specular;

    HairVertex*          vertexBuffer;
    HairAttributes*      attribBuffer;
    HairStrandDesc*      strandDescBuffer;
    GPUCameraData*       cameraBuffer;
    GPULightData*        lights;
    ScreenSpaceTriangle* trianglesBuffer;
    float4*              debugColorBuffer;
    
    uint                 hairFirstVertex;
    uint                 hairVertexCount;
    uint                 hairFirstStrand;
    uint                 hairStrandCount;

    uint*                smallTriangleCountBuffer;
    uint                 maxSmallTriangles;
    float                smallTriangleThreshold;
    float                width;
    float                height;

    float                specularFactor;
    int32_t              useCustomColor;

    int32_t              isHybrid;

    // Marschner BSDF Params
    MarschnerParams      bsdf;

    int32_t              vizMode;
};

struct VisibilityPushConstants
{
    ScreenSpaceTriangle* trianglesBuffer;
    uint*                smallTriangleCountBuffer;
    float                width;
    float                height;
};

struct ResolvePushConstants
{
    float3* colorsBuffer;
    uint    colorCount;
    float   width;
    float   height;
};

/**
 * Fetch the strand description for the specified index from the global storage buffer.
 */
HairStrandDesc getStrandDescription(MeshPushConstants pc, uint globalStrandIndex)
{
    // A single buffer is used for all hair models, we need the global index for that buffer.
    uint index = pc.hairFirstStrand + globalStrandIndex;
    return pc.strandDescBuffer[index];
}

float getSignedArea(float2 a, float2 b, float2 c)
{
    float2 AB = float2(b.x - a.x, b.y - a.y);
    float2 AC = float2(c.x - a.x, c.y - a.y);
    return 0.5 * determinant(float2x2(AB, AC));
}

float2 toScreen(float4 clipPos, float2 viewportSize)
{
    float2 ndc = clipPos.xy / clipPos.w;
    return (ndc * 0.5 + 0.5) * viewportSize;
}

bool isSmallTriangle(float2 a, float2 b, float2 c, float threshold)
{
    float2 bbMin = min(min(a, b), c);
    float2 bbMax = max(max(a, b), c);
    float2 size  = bbMax - bbMin;
    return max(size.x, size.y) < threshold;
}