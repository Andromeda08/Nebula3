#pragma once

#include "types.slang"

static const float EPSILON  = 1e-4;

static const float T_MIN    = EPSILON;
static const float T_MAX    = 1e30;

static const float PI     = float.getPi();
static const float INV_PI = 1.0 / PI;
static const float TWO_PI = 2.0 * PI;

struct Ray
{
    float3 origin;
    float3 dir;

    float3 r(float t)
    {
        return origin + dir * t;
    }

    RayDesc getRayDesc(float tMin, float tMax)
    {
        RayDesc r;
        r.Origin    = origin;
        r.Direction = dir;
        r.TMin      = tMin;
        r.TMax      = tMax;
        return r;
    }
};

struct PathPayload
{
    float3 throughput;
    float3 radiance;

    Ray ray;

    int  depth;
    uint seed;

    float prevBsdfPdf;
    bool  prevSpecular;
    bool  isTerminated;

    float pathLength;

    float3 gNormal;
    float3 gDiffuse;
    float3 gSpecular;
    float  gRoughness;
    float  gDepth;
    float3 gWorldPos;
    int    gInstanceId;
    bool   gWritten;
    float  gSpecHitDist;

    bool   primaryHitSpecular;
};

struct PathTracerPushConstants
{
    GPUCameraData*      camera;        // Camera BDA
    GPUCameraData*      prevCamera;    // Camera BDA
    GPUInstanceData*    instances;     // Instance BDA
    AreaEmitter*        emitters;      // Emitters BDA
    float*              emitterPdfs;   // Emitter Discrete PDF BDA
    Vertex*             vertices;      // Vertex BDA
    uint*               indices;       // Index BDA
    GPUMaterialData*    materials;     // Materials BDA
    GPUGeometryInfo*    geometryInfos; // Geometry Info BDA
    uint64_t            accumulated;   // Current accumulation frame counter
    uint64_t            totalFrames;   // Total lifetime frame counter
    uint                maxBounces;    // Maximum number of bounces before RR
    uint                spp;           // Samples-per-pixel
    int                 bDynamicRR;    // Use dynamic rr continuation probability based on throughput?
    float               rrCont;        // RR continuation probability if !bDynamicRR
    uint                emitterCount;  // N emitters
    float               jitterX;
    float               jitterY;
};

float getLuminance(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float balanceHeuristic(float a, float b)
{
    return a / (a + b);
}

uint pcgHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint initSeed(uint2 pixel, uint frame)
{
    uint h = pcgHash(pixel.x + pcgHash(pixel.y + pcgHash(frame)));
    return h;
}

float rnd(inout uint seed)
{
    seed = pcgHash(seed);
    return float(seed >> 8) * (1.0 / 16777216.0);
}

float2 rnd2(inout uint seed) { return float2(rnd(seed), rnd(seed)); }
float3 rnd3(inout uint seed) { return float3(rnd(seed), rnd(seed), rnd(seed)); }
