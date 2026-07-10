#pragma once

float3 toCosineHemisphere(float2 u)
{
    const float r = sqrt(u.x);
    const float t = 2.0 * float.getPi() * u.y;
    return float3(r * cos(t), r * sin(t), sqrt(1.0 - u.x));
}

float getCosineHemispherePdf(float3 v)
{
    return (v.z >= 0.0) ? v.z * (1.0 / float.getPi()) : 0.0;
}
