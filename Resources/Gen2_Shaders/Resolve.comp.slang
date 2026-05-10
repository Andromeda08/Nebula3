#language slang 2026

struct DebugColor
{
    float3 color;
};

struct PushConstants
{
    DebugColor* colors;
    uint        width;
    uint        height;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

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
    if (any(pixel >= uint2(pc.width, pc.height)))
        return;

    uint64_t packed = gVisBuffer[int2(pixel)];
    uint primId = uint(packed & 0xFFFFFFFFu);

    float3 color;
    if (primId == gInvalidPrimitiveId)
    {
        color = float3(0.0, 0.0, 0.0);
    }
    else
    {
        color = pc.colors[primId].color;
    }

    gColorTarget[int2(pixel)] = float4(color, 1.0);
}