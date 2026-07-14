#language slang 2026

#include "../Path/lib/types.slang"

struct PushConstants
{
    GPUInstanceData* instances;
    GPUCameraData*   camera;
    int*             objectID;
    float2           mousePos;
    float2           screenSize;
};

[[vk::push_constant]] PushConstants pc;

[[vk::binding(0, 0)]] RWTexture2D<int2> uObjectInstanceBuffer;

[shader("compute")]
[NumThreads(1, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    let pixel = int2(floor(pc.mousePos));

    if (any(pixel < int2(0)) || any(pixel >= int2(pc.screenSize)))
    {
        *pc.objectID = -1;
        return;
    }

    *pc.objectID = uObjectInstanceBuffer[pixel].x;
}
