#pragma once

#include "frame.slang"
#include "types.slang"

struct SurfacePoint
{
    float3 pos;
    float3 N;
    float4 T;

    Frame  shFrame;
    float3 geoN;
};

struct Triangle
{
    // Object-space vertices and indices
    uint   i0, i1, i2;
    Vertex v0, v1, v2;
    float  area;

    // World-space positions
    float3 p0, p1, p2;
    float  worldArea;

    // Geometric Normal is in world-space
    float3 geometricNormal;

    static Triangle reconstruct(float4x4 model, uint firstIndex, uint firstVertex, uint* indexBuffer, Vertex* vertexBuffer)
    {
        Triangle tri;

        tri.i0 = indexBuffer[firstIndex + 0] + firstVertex;
        tri.v0 = vertexBuffer[tri.i0];

        tri.i1 = indexBuffer[firstIndex + 1] + firstVertex;
        tri.v1 = vertexBuffer[tri.i1];

        tri.i2 = indexBuffer[firstIndex + 2] + firstVertex;
        tri.v2 = vertexBuffer[tri.i2];

        tri.area = 0.5 * length(cross(tri.v1.position - tri.v0.position, tri.v2.position - tri.v0.position));

        tri.p0 = mul(model, float4(tri.v0.position, 1.0)).xyz;
        tri.p1 = mul(model, float4(tri.v1.position, 1.0)).xyz;
        tri.p2 = mul(model, float4(tri.v2.position, 1.0)).xyz;

        var wc  = cross(tri.p1 - tri.p0, tri.p2 - tri.p0);
        tri.worldArea = 0.5 * length(wc);
        tri.geometricNormal = normalize(wc);

        return tri;
    }

    SurfacePoint getWorldSurfacePoint(float4x4 model, float2 bary)
    {
        var bc = float3(1.0 - bary.x - bary.y, bary.x, bary.y);

        float3 N   = normalize(v0.normal * bc.x + v1.normal * bc.y + v2.normal * bc.z);
        float4 T   = v0.tangent * bc.x + v1.tangent * bc.y + v2.tangent * bc.z;

        var nm = (float3x3) model;

        SurfacePoint p;

        p.pos     = p0 * bc.x + p1 * bc.y + p2 * bc.z;
        p.N       = normalize(mul(nm, N));
        p.T       = float4(mul(nm, T.xyz), T.w);
        p.shFrame = Frame::build(p.N, p.T);
        p.geoN    = geometricNormal;

        return p;
    }

    float3 samplePosition(float2 sample)
    {
        var u = sqrt(sample.x);
        var b = float2(1.0 - u, sample.y * u);

        return p0 * b.x + p1 * b.y + p2 * (u - b.y);
    }
};
