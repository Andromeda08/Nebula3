#include "lib/nbl.slang"

[[vk::binding(0)]] RaytracingAccelerationStructure uTopLevelAS;

struct PushConstants
{
    GPUInstanceData*  instances;
    GPUCameraData*    camera;
    int*              resultBuffer;
    float2            mousePos;
    float2            screenSize;
};

[[vk::push_constant]] PushConstants pc;

[shader("compute"), numthreads(1, 1, 1), require(spvRayQueryKHR)]
void main(uint3 id : SV_DispatchThreadID)
{
    const float2 ndcMousePos = float2(
        (2.0 * pc.mousePos.x) / pc.screenSize.x - 1.0,
        (2.0 * pc.mousePos.y) / pc.screenSize.y - 1.0
    );

    float4 farView = mul(pc.camera.projInverse, float4(ndcMousePos, 1.0, 1.0));
    farView /= farView.w;
    const float3 farWorld = mul(pc.camera.viewInverse, float4(farView.xyz, 1.0)).xyz;

    const float3 origin = pc.camera.eye.xyz;
    const float3 dir    = normalize(farWorld - origin);

    RayDesc ray;
    ray.Origin    = origin;
    ray.TMin      = pc.camera.nearPlane;
    ray.Direction = dir;
    ray.TMax      = pc.camera.farPlane;

    RayQuery<RAY_FLAG_FORCE_OPAQUE> rq;
    rq.TraceRayInline(uTopLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    while (rq.Proceed()) {}

    if (rq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        const uint idx = rq.CommittedInstanceID();

        GPUInstanceData instance = pc.instances[idx];
        *pc.resultBuffer = instance.objectId;
    }
    else
    {
        *pc.resultBuffer = -1;
    }
}
