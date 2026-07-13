#language slang 2026

#include "lib/bsdf.slang"
#include "lib/common.slang"
#include "lib/emitter.slang"
#include "lib/frame.slang"
#include "lib/sampling.slang"
#include "lib/triangle.slang"

[[vk::binding(0, 2)]] Sampler2D uTextures[1024];

float3 offsetRay(float3 p, float3 n)
{
    const float origin     = 1.0 / 32.0;
    const float floatScale = 1.0 / 65536.0;
    const float intScale   = 256.0;

    int3   of_i = int3(intScale * n);
    float3 p_i  = asfloat(asint(p) + select(p < 0.0, -of_i, of_i));

    return float3(
        abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
        abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
        abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}

bool occluded(float3 origin, float3 dir, float tMax)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> q;
    RayDesc rd;
    rd.Origin = origin; rd.Direction = dir; rd.TMin = 1e-3; rd.TMax = tMax;
    q.TraceRayInline(uTLAS, RAY_FLAG_NONE, 0xFF, rd);
    q.Proceed();
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

[[vk::push_constant]] PathTracerPushConstants pc;

[[vk::binding(0, 1)]] RaytracingAccelerationStructure uTLAS;

[shader("closesthit")]
void main(
    inout PathPayload                           path,
    in    BuiltInTriangleIntersectionAttributes attr
)
{
    uint instanceId = InstanceID();
    uint primId     = PrimitiveIndex();

    GPUInstanceData instance = pc.instances[instanceId];
    GPUGeometryInfo geomInfo = pc.geometryInfos[instance.geometryIndex];
    GPUMaterialData material = pc.materials[instance.materialIndex];

    // Triangle Fetch
    uint base = geomInfo.firstIndex + primId * 3;
    var  tri  = Triangle::reconstruct(instance.model, base, geomInfo.firstVertex, pc.indices, pc.vertices);
    var  hit  = tri.getWorldSurfacePoint(instance.model, attr.barycentrics);

    float3 worldWi = -WorldRayDirection();

    if (dot(hit.N, worldWi)  < 0.0) hit.N    = -hit.N;
    if (dot(hit.geoN, hit.N) < 0.0) hit.geoN = -hit.geoN;

    float3 wi = hit.shFrame.toLocal(worldWi);

    // Direct emitter hit ([tri] and [hit] are valid inside)
    if (instance.isEmitter())
    {
        float3 Le = pc.emitters[instance.emitterIndex].radiance;
        float wBSDF = 1.0;

        if (path.depth > 0 && !path.prevSpecular)
        {
            float cosLight = abs(dot(hit.geoN, WorldRayDirection()));
            float dist     = RayTCurrent();
            float lightPdf = emitterPdfSurfaceArea(pc.emitters[instance.emitterIndex], pc, primId, tri.worldArea, dist, cosLight);
            wBSDF = balanceHeuristic(path.prevBsdfPdf, lightPdf);
        }

        path.radiance += path.throughput * Le * wBSDF;
    }

    float3 albedo = material.solidColor.rgb;
    if (material.textureIndex >= 0)
    {
        albedo = uTextures[NonUniformResourceIndex(material.textureIndex)].SampleLevel(hit.uv, 0.0).rgb;
    }

    // BSDF Sampling
    BSDF::Record bRec = BSDF::sample(material, path.seed, wi, albedo);
    path.seed = bRec.seed;

    if (all(bRec.value == 0.0))
    {
        path.isTerminated = true;
        return;
    }

    if (path.depth == 0)
    {
        float3 viewPos = mul(pc.camera.view, float4(hit.pos, 1.0)).xyz;

        path.gNormal            = hit.N;
        path.gWorldPos          = hit.pos;
        path.gInstanceId        = int(instanceId);
        path.gDepth             = -viewPos.z;
        path.gDiffuse           = bRec.isDiscrete ? float3(0.0) : albedo;
        path.gSpecular          = bRec.isDiscrete ? float3(1.0) : float3(0.04);
        path.gRoughness         = bRec.isDiscrete ? 0.0 : 1.0;
        path.primaryHitSpecular = bRec.isDiscrete;
        path.gWritten           = true;
    }
    else if (path.depth == 1 && path.primaryHitSpecular)
    {
        path.gSpecHitDist = RayTCurrent();
    }

    // NEE
    if (!bRec.isDiscrete && pc.emitterCount > 0)
    {
        // Pick a light
        LightSample ls = sampleEmitter(hit.pos, pc, path.seed);
        if (ls.isValid)
        {
            float3 woLocal = hit.shFrame.toLocal(ls.wi);
            float  cosSurf = woLocal.z;
            if (cosSurf > 0.0)
            {
                BSDF::Params bParams;
                bParams.wi = wi;
                bParams.wo = woLocal;
                float3 f = BSDF::eval(material, path.seed, bParams, albedo);

                if (any(f > 0.0))
                {
                    float  sign = (dot(ls.wi, hit.geoN) < 0.0) ? -1.0 : 1.0;
                    float3 so   = offsetRay(hit.pos, hit.geoN * sign);

                    if (!occluded(so, ls.wi, ls.dist * (1.0 - EPSILON)))
                    {
                        float bsdfPdf    = BSDF::pdf(material, path.seed, bParams);
                        float w          = balanceHeuristic(ls.pdf, bsdfPdf);

                        float3 contrib = f * cosSurf * ls.Le / ls.pdf;
                        path.radiance += path.throughput * contrib * w;
                    }
                }
            }
        }
    }

    float3 woWorld = hit.shFrame.toWorld(bRec.params.wo);
    float  sign    = (dot(woWorld, hit.geoN) < 0.0) ? -1.0 : 1.0;

    // Update path
    path.throughput  *= bRec.value;
    path.depth       += 1;
    path.pathLength  += RayTCurrent();
    path.ray.origin   = offsetRay(hit.pos, hit.geoN * sign);
    path.ray.dir      = woWorld;
    path.prevBsdfPdf  = bRec.isDiscrete ? 0.0 : bRec.pdf;
    path.prevSpecular = bRec.isDiscrete;
    path.isTerminated = false;
}
