#language slang 2026

#include "nbl/Hair.inc.slang"

[[vk::push_constant]]
ConstantBuffer<ResolvePushConstants> pc;

[[vk::binding(0, 0)]]
RWTexture2D<uint64_t> gVisBuffer;

[[vk::binding(1, 0)]]
RWTexture2D<float4> gColorTarget;

static const uint gInvalidPrimitiveId = 0xFFFFFFFFu;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID: SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;
    if (any(pixel >= uint2(uint(pc.width), uint(pc.height))))
    {
        return;
    }

    uint64_t packed = gVisBuffer[int2(pixel)];
    uint primId = uint(packed & 0xFFFFFFFFu);

    float3 color;
    if (primId == gInvalidPrimitiveId)
    {
        color = float3(0.0, 0.0, 0.0);
    }
    else
    {
        color = pc.colorsBuffer[primId % pc.colorCount].rgb;
    }

    gColorTarget[int2(pixel)] = float4(color, 1.0);
}