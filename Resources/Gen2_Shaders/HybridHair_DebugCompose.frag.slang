#language slang 2026

[[vk::binding(0, 0)]]
Sampler2D gMeshOutput;

[[vk::binding(1, 0)]]
Sampler2D gComputeOutput;

[[vk::push_constant]]
ConstantBuffer<float> gAlphaBlend;

[shader("fragment")]
float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float4 a = gMeshOutput.Sample(uv);
    float4 b = gComputeOutput.Sample(uv);

    float3 rgb = lerp(a.rgb, b.rgb, b.a * gAlphaBlend);
    return float4(rgb, 1.0);
}
