#language slang 2026

#include "lib/nbl.slang"

struct AABBDebugPushConstants
{
    float4           color;
    GPUInstanceData* instances;
    GPUCameraData*   camera;
    uint             instanceGpuIndex;
};

[[vk::push_constant]] AABBDebugPushConstants pc;

static const int2 edges[12] = {
    int2(0, 1), int2(1, 3), int2(3, 2), int2(2, 0),
    int2(4, 5), int2(5, 7), int2(7, 6), int2(6, 4),
    int2(0, 4), int2(1, 5), int2(3, 7), int2(2, 6)
};

float3 getPosition(float3 aabbMin, float3 aabbMax, int vertexIndex)
{
    const int edgeIndex = vertexIndex / 2;
    const int vi        = (vertexIndex % 2 == 0) ? edges[edgeIndex].x : edges[edgeIndex].y;

    const float3 a = float3(float((vi & 1) != 0), float((vi & 2) != 0), float((vi & 4) != 0));

    return lerp(aabbMin, aabbMax, a);
}

[shader("vertex")]
float4 vsMain(uint vertexID : SV_VertexID) : SV_Position
{
    GPUInstanceData inst = pc.instances[pc.instanceGpuIndex];
    const var       pos  = getPosition(inst.aabbMin.xyz, inst.aabbMax.xyz, int(vertexID));

    return mul(pc.camera.proj, mul(pc.camera.view, float4(pos, 1.0)));
}

[shader("fragment")]
float4 main() : SV_Target0
{
    return pc.color;
}
