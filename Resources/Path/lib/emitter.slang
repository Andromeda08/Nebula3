#pragma once

#include "common.slang"
#include "triangle.slang"
#include "types.slang"

struct LightSample
{
    float3 wi;
    float3 Le;
    float  dist;
    float  pdf;
    bool   isValid;
};

uint pickTriangle(float* emitterPdfs, uint cdfOffset, uint triCount, float u)
{
    for (uint i = 0; i < triCount; ++i)
    {
        if (u < emitterPdfs[cdfOffset + i + 1])
        {
            return i;
        }
    }
    return triCount - 1;
}

float emitterAreaPdf(AreaEmitter emitter, PathTracerPushConstants pc, uint triIndex, float worldArea)
{
    float selPdf       = 1.0 / float(pc.emitterCount);
    float triSelectPdf = pc.emitterPdfs[emitter.cdfOffset + triIndex + 1] - pc.emitterPdfs[emitter.cdfOffset + triIndex];
    return selPdf * triSelectPdf / worldArea;
}

float emitterPdfSurfaceArea(AreaEmitter emitter, PathTracerPushConstants pc, uint primId, float worldArea, float dist, float cosTheta)
{
    return emitterAreaPdf(emitter, pc, primId, worldArea) * dist * dist / max(cosTheta, 1e-6);
}

LightSample sampleEmitter(float3 refPos, PathTracerPushConstants pc, inout uint seed)
{
    LightSample s;
    s.isValid = false;

    uint emitterIndex = min(uint(rnd(seed) * pc.emitterCount), pc.emitterCount - 1);
    var  emitter      = pc.emitters[emitterIndex];
    var  triIndex     = pickTriangle(pc.emitterPdfs, emitter.cdfOffset, emitter.triCount, rnd(seed));

    GPUGeometryInfo geomInfo = pc.geometryInfos[emitter.geometryIndex];
    float4x4        model    = pc.instances[emitter.instanceIndex].model;

    uint base = geomInfo.firstIndex + triIndex * 3;
    var  tri  = Triangle::reconstruct(model, base, geomInfo.firstVertex, pc.indices, pc.vertices);

    float2 u   = rnd2(seed);
    var    pos = tri.samplePosition(u);

    float3 N    = tri.geometricNormal;

    float3 toL  = pos - refPos;
    float  dist = length(toL);
    float3 L    = toL / dist;

    float cosTheta = dot(N, -L);
    if (cosTheta <= 0.0)
    {
        return s;
    }

    s.wi      = L;
    s.Le      = emitter.radiance;
    s.dist    = dist;
    s.pdf     = emitterPdfSurfaceArea(emitter, pc, triIndex, tri.worldArea, dist, cosTheta);
    s.isValid = true;

    return s;
}
