#language slang 2026

#include "nbl/Hair.inc.slang"

[[vk::push_constant]]
ConstantBuffer<MeshPushConstants> pc;

float3 kajiyaKay(float3 diffuse, float3 specular, float p, float3 T, float3 L, float3 V)
{
    float cosTL = dot(T, L);
    float sinTL = sqrt(1.0 - cosTL * cosTL);

    float3 H = normalize(L + V);
    float cosTH = dot(T, H);
    float sinTH = sqrt(1.0 - cosTH * cosTH);

    float3 d = diffuse * sinTL;
    float3 s = specular * pow(max(sinTH, 0.0), p);

    return d + s;
}

[shader("fragment")]
float4 main(MeshVertex input) : SV_Target
{
    const float3 lightPos = float3(-75.0, 125.0, 50.0);

    float3 T = normalize(input.worldTangent);
    float3 L = normalize(lightPos - input.worldPosition);
    float3 V = normalize(pc.cameraBuffer.eye.xyz - input.worldPosition);

    float3 diffuse  = pc.diffuse.rgb; // pc.useCustomColor == 1 ? pc.diffuse.rgb : input.color;
    float3 specular = pc.specular.rgb; // pc.useCustomColor == 1 ? pc.specular.rgb : input.color;

    float3 color = kajiyaKay(diffuse, specular, 16.0, T, L, V);
    return float4(color, 1.0);
}
