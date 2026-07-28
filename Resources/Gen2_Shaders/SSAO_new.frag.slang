#language slang 2026

#include "lib/nbl.slang"

namespace SSAO
{
    static const int   gKernelSize = 32;
    static const float gRadius     = 4.0;
    static const float gExposure   = 4.0;
    static const float gBias       = 0.025;
}

[[vk::binding(0, 0)]] Sampler2D uNoiseTex;
[[vk::binding(1, 0)]] Sampler2D uWorldPosition;
[[vk::binding(2, 0)]] Sampler2D uWorldNormal;
[[vk::binding(3, 0)]] Sampler2D uViewZ;

struct SSAOPushConstants
{
    GPUCameraData*  camera;
    float4*         kernel;
    int2            renderDim;
    int2            noiseDim;
};
[[vk::push_constant]] SSAOPushConstants pc;

[shader("fragment")]
float frag(float2 uv)
{
    var viewZ = uViewZ.Sample(uv).r;
    if (viewZ >= 0.0)
    {
        return 1.0;
    }

    var worldPosition = uWorldPosition.Sample(uv).xyz;
    var worldNormal   = uWorldNormal.Sample(uv).xyz;

    var viewPosition  = pc.camera.worldToView_Point(worldPosition);
    var viewNormal    = pc.camera.worldToView_Vector(worldNormal);

    var noiseScale = float2(pc.renderDim) / float2(pc.noiseDim);
    var randomVec  = uNoiseTex.Sample(uv * noiseScale).xyz * 2.0 - 1.0;

    var T = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    var B = cross(viewNormal, T);
    var TBN = float3x3(T, B, viewNormal);

    float occlusion = 0.0;

    for (int i = 0; i < SSAO::gKernelSize; i++)
    {
        var samplePos = float4(viewPosition + mul(TBN, pc.kernel[i].xyz) * SSAO::gRadius, 1.0);
        var offset = mul(pc.camera.proj, samplePos);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        var sampleUV = clamp(offset.xy, 0.0, 1.0);
        var depth    = uViewZ.Sample(sampleUV).r;

        var rangeCheck = smoothstep(0.0, 1.0, SSAO::gRadius / abs(viewZ - depth));
        occlusion += (depth > samplePos.z + SSAO::gBias ? 1.0 : 0.0) * rangeCheck;
    }

    var factor = 1.0 - (occlusion / float(SSAO::gKernelSize));
    return pow(factor, SSAO::gExposure);
}
