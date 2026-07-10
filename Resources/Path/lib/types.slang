#pragma once

struct GPUInstanceData
{
    float4x4    model;
    float4x4    modelInverse;
    float4x4    modelPrevious;
    float4      aabbMin;
    float4      aabbMax;
    uint64_t    blas;
    int         geometryIndex;
    int         materialIndex;
    int         objectId;
    int         emitterIndex;

    bool isEmitter() { return emitterIndex >= 0; }
};

struct GPUGeometryInfo
{
    int   geometryIndex;
    uint  triangleCount;

    // VertexBuffer Region
    uint  firstVertex;
    uint  vertexCount;

    // IndexBuffer Region
    uint  firstIndex;
    uint  indexCount;
};

struct GPUCameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4   eye;
    float4   frustumPlanes[6];
    float    nearPlane;
    float    farPlane;
};

struct AreaEmitter
{
    uint   instanceIndex;
    int    geometryIndex;
    uint   cdfOffset;
    uint   triCount;
    float  totalWeight;
    float3 radiance;
};

struct GPUMaterialData
{
    float4  solidColor;
    int     textureIndex;
    int     normalMapIndex;
    int     metallicRoughnessMapIndex;
    float   metallicFactor;
    float   roughnessFactor;
    int     isEmissive;
    uint    rtHitGroup;

    uint    bsdfIndex;
    float4  diffuse_Albedo;
    float   dielectric_IntIoR;
    float   dielectric_ExtIoR;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};