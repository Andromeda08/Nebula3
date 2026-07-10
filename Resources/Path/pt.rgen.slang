#language slang 2026

#include "lib/common.slang"

[[vk::push_constant]] PathTracerPushConstants pc;

[[vk::binding(0, 0)]] RWTexture2D<float4> uOutImage;
[[vk::binding(1, 0)]] RWTexture2D<float4> uAccumRead;
[[vk::binding(2, 0)]] RWTexture2D<float4> uAccumWrite;

[[vk::binding(0, 1)]] RaytracingAccelerationStructure uTLAS;

Ray getJitteredCameraRay(uint seed)
{
    float2 jitter = rnd2(seed);
    float2 uv     = (float2(DispatchRaysIndex().xy) + jitter) / float2(DispatchRaysDimensions().xy);
    float2 ndc    = uv * 2.0 - 1.0;

    float4 target = mul(pc.camera.projInverse, float4(ndc, 1.0, 1.0));

    Ray ray;

    ray.origin = mul(pc.camera.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    ray.dir = mul(pc.camera.viewInverse, float4(normalize(target.xyz), 0.0)).xyz;

    return ray;
}

PathPayload getInitialPayload(uint seed, Ray ray)
{
    PathPayload path;

    path.throughput   = float3(1.0);
    path.radiance     = float3(0.0);
    path.ray          = ray;
    path.depth        = 0;
    path.seed         = seed;
    path.prevBsdfPdf  = 0.0;
    path.prevSpecular = true;
    path.isTerminated = false;

    return path;
}

[shader("raygeneration")]
void main()
{
    uint2 pixel = DispatchRaysIndex().xy;
    uint2 dim   = DispatchRaysDimensions().xy;

    float3 radiance = float3(0.0);

    for (uint s = 0; s < pc.spp; s++)
    {
        uint seed = initSeed(pixel, (uint)pc.totalFrames * pc.spp + s);

        Ray         ray  = getJitteredCameraRay(seed);
        PathPayload path = getInitialPayload(seed, ray);

        for (uint b = 0; b < pc.maxBounces; b++)
        {
            RayDesc r = path.ray.getRayDesc(EPSILON, 1e30);
            TraceRay(uTLAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 2, 0, r, path);

            if (path.isTerminated)
            {
                break;
            }

            // Russian roulette
            if (b > 3)
            {
                float q = (pc.bDynamicRR == 1) ? max(path.throughput.r, max(path.throughput.g, path.throughput.b)) : pc.rrCont;
                if (rnd(path.seed) > q)
                {
                    break;
                }
                path.throughput /= q;
            }
        }

        radiance += path.radiance;
    }

radiance /= float(pc.spp);

    uint prevSamples = pc.accumulated * pc.spp;
    uint newSamples  = prevSamples + pc.spp;

    float3 avg;
    if (pc.accumulated == 0)
        avg = radiance;
    else
    {
        float3 prev = uAccumRead[pixel].rgb;
        avg = (prev * float(prevSamples) + radiance * float(pc.spp)) / float(newSamples);
    }

    uAccumWrite[pixel] = float4(avg, 1.0);
    uOutImage[pixel]   = float4(avg, 1.0);
}