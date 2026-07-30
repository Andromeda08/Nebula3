#pragma once

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
