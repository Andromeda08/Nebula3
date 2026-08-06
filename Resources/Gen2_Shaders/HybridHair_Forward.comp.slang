#include "nbl/Hair.inc.slang"

#include "lib/HybridHair.slang"

struct RasterizerPushConstants
{
    GPUCameraData*       camera;
    GPULightData*        lights;
    ScreenSpaceTriangle* triangles;
    uint*                triangleCount;
    float2               viewportSize;

    MarschnerParams      bsdf;

    BBoxInt2 getScissor()
    {
        return BBoxInt2::fromScissor(0, 0, uint(pc.viewportSize.x), uint(pc.viewportSize.y));
    }
};

[[vk::push_constant]] RasterizerPushConstants pc;

[[vk::binding(0, 0)]] RWTexture2D<float4> uColorTarget;
[[vk::binding(1, 0)]] RWTexture2D<float>  uDepthBuffer;

[shader("compute"), numthreads(64, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    const uint triangleCount = pc.triangleCount[0];

    // Triangle fetch
    uint triIndex = threadId.x;
    if (triIndex >= triangleCount) return;

    ScreenSpaceTriangle t   = pc.triangles[triIndex];
    Triangle            tri = Triangle::fromInput(t);

    // Scissor
    BBoxInt2 clipped = tri.bounds.getClippedBy(pc.getScissor());
    if (clipped.isEmpty()) return;

    // Loop over pixels
    for (int y = clipped.bboxMin.y; y <= clipped.bboxMax.y; y++)
    {
        for (int x = clipped.bboxMin.x; x <= clipped.bboxMax.x; x++)
        {
            var coord = int2(x, y);
            var pixel = float2(x, y) + 0.5;
            var bary  = tri.bary(pixel);
            if (any(bary < 0.0f))
            {
                continue;
            }

            float3 interp    = (bary.x * t.v0) + (bary.y * t.v1) + (bary.z * t.v2);
            float2 screenPos = interp.xy;
            float  depth     = interp.z;

            // Depth test
            if (depth >= uDepthBuffer[coord]) continue;
            // float2 ndcPos = (screenPos / pc.viewportSize) * 2.0 - 1.0;

            // Reconstruct World space params
            // float4 viewPosH = mul(pc.camera.viewInverse, float4(ndcPos, depth, 1.0));
            // float3 viewPos  = viewPosH.xyz / viewPosH.w;
            // float3 worldPos = mul(pc.camera.viewInverse, float4(viewPos, 1.0)).xyz;

            float3 worldPos = bary.x * t.w0 + bary.y * t.w1 + bary.z * t.w2;

            // BSDF, width default light at index 0.
            float3 color = MarschnerBSDF(pc.lights[0], pc.camera[0], worldPos, t.tangent.xyz, pc.bsdf);

            // Write
            uColorTarget[coord] = float4(color, 1.0);
            uDepthBuffer[coord] = depth;
        }
    }
}
