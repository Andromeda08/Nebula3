#pragma once

struct Vertex
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
}

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
