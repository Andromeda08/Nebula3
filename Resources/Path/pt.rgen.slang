#language slang 2026

#include "lib/common.slang"

[[vk::push_constant]] PathTracerPushConstants pc;

// 1 SPP Output
[[vk::binding(0, 0)]] RWTexture2D<float4> uOutImage;

// Temporal Accumulation
// [[vk::binding(1, 0)]] RWTexture2D<float4> uAccumRead;
// [[vk::binding(2, 0)]] RWTexture2D<float4> uAccumWrite;

// G-Buffer
[[vk::binding(1, 0)]] RWTexture2D<float4> uNormal;
[[vk::binding(2, 0)]] RWTexture2D<float4> uDiffuse;
[[vk::binding(3, 0)]] RWTexture2D<float4> uSpecular;
[[vk::binding(4, 0)]] RWTexture2D<float>  uRoughness;
[[vk::binding(5, 0)]] RWTexture2D<float>  uDepth;
[[vk::binding(6, 0)]] RWTexture2D<float2> uMotion;
[[vk::binding(7, 0)]] RWTexture2D<float>  uSpecularHitDist;

[[vk::binding(0, 1)]] RaytracingAccelerationStructure uTLAS;

Ray getJitteredCameraRay_NonHalton(uint seed)
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

Ray getJitteredCameraRay()
{
    float2 jitter = float2(pc.jitterX, pc.jitterY);
    float2 uv     = (float2(DispatchRaysIndex().xy) + 0.5 + jitter) / float2(DispatchRaysDimensions().xy);
    float2 ndc    = uv * 2.0 - 1.0;

    float4 target = mul(pc.camera.projInverse, float4(ndc, 1.0, 1.0));

    Ray ray;
    ray.origin = mul(pc.camera.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    ray.dir    = mul(pc.camera.viewInverse, float4(normalize(target.xyz), 0.0)).xyz;
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
    path.pathLength   = 0.0;

    path.gNormal            = float3(0.0, 0.0, 1.0);
    path.gDiffuse           = float3(0.0);
    path.gSpecular          = float3(0.0);
    path.gRoughness         = 1.0;
    path.gDepth             = T_MAX;
    path.gWorldPos          = float3(0.0);
    path.gInstanceId        = -1;
    path.gWritten           = false;
    path.gSpecHitDist       = 0.0;
    path.primaryHitSpecular = false;

    return path;
}

float2 computeMotion(float3 worldPos, int instanceId, uint2 dim)
{
    if (instanceId < 0) return float2(0.0);

    GPUInstanceData inst = pc.instances[instanceId];

    float3 prevWorld = worldPos;
    if (any(inst.modelPrevious[0] != inst.model[0]) ||
        any(inst.modelPrevious[1] != inst.model[1]) ||
        any(inst.modelPrevious[2] != inst.model[2]) ||
        any(inst.modelPrevious[3] != inst.model[3]))
    {
        float3 objPos = mul(inst.modelInverse,  float4(worldPos, 1.0)).xyz;
        prevWorld     = mul(inst.modelPrevious, float4(objPos,   1.0)).xyz;
    }

    float4x4 curVP  = mul(pc.camera.proj,     pc.camera.view);
    float4x4 prevVP = mul(pc.prevCamera.proj, pc.prevCamera.view);

    float4 curClip  = mul(curVP,  float4(worldPos,  1.0));
    float4 prevClip = mul(prevVP, float4(prevWorld, 1.0));

    float2 curNDC  = curClip.xy  / curClip.w;
    float2 prevNDC = prevClip.xy / prevClip.w;

    return (prevNDC - curNDC) * 0.5 * float2(dim);
}

uint xxhash32(uint3 p)
{
    const uint4 primes = uint4(2246822519u, 3266489917u, 668265263u, 374761393u);
    uint h32;
    h32 = p.z + primes.w + p.x * primes.y;
    h32 = primes.z * ((h32 << 17) | (h32 >> 15));
    h32 += p.y * primes.y;
    h32 = primes.z * ((h32 << 17) | (h32 >> 15));
    h32 = primes.x * (h32 ^ (h32 >> 15));
    h32 = primes.y * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

[shader("raygeneration")]
void main()
{
    uint2 pixel = DispatchRaysIndex().xy;
    uint2 dim   = DispatchRaysDimensions().xy;

    float3 radiance = float3(0.0);

    // for (uint s = 0; s < pc.spp; s++)
    // {
        // uint seed = initSeed(pixel, (uint)pc.totalFrames + s);
        // Ray         ray  = getJitteredCameraRay(seed);
        // PathPayload path = getInitialPayload(seed, ray);

        uint seed = xxhash32(uint3(pixel.xy, (uint)pc.totalFrames));

        // uint        seed = initSeed(pixel, (uint)pc.totalFrames);
        Ray         ray  = getJitteredCameraRay();
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
    // }

    uOutImage[pixel]        = float4(path.radiance, 1.0);
    uNormal[pixel]          = float4(path.gNormal, 0.0);
    uDiffuse[pixel]         = float4(path.gDiffuse, 1.0);
    uSpecular[pixel]        = float4(path.gSpecular, 1.0);
    uRoughness[pixel]       = path.gRoughness;
    uDepth[pixel]           = path.gDepth;
    uMotion[pixel]          = computeMotion(path.gWorldPos, path.gInstanceId, dim);
    uSpecularHitDist[pixel] = path.gSpecHitDist;

    // radiance /= float(pc.spp);

    // uint prevSamples = pc.accumulated * pc.spp;
    // uint newSamples  = prevSamples + pc.spp;

    // float3 avg;
    // if (pc.accumulated == 0)
    //     avg = radiance;
    // else
    // {
    //     float3 prev = uAccumRead[pixel].rgb;
    //     avg = (prev * float(prevSamples) + radiance * float(pc.spp)) / float(newSamples);
    // }

    // uAccumWrite[pixel] = float4(avg, 1.0);
    // uOutImage[pixel]   = float4(avg, 1.0);
}