#pragma once

static const float hairIoR = 1.55;

struct MarschnerParams
{
    float  roughness;
    float  azimuthalWidth;
    float3 absorption;
    float  shiftR;
    float  shiftTT;
    float  shiftTRT;
    float  scaleR;
    float  scaleTT;
    float  scaleTRT;
};

float3x3 makeHairFrame(float3 T)
{
    float3 B = normalize(cross(T, float3(0.0, 1.0, 0.0)));
    if (length(B) < 0.001)
    {
        B = normalize(cross(T, float3(1.0, 0.0, 0.0)));
    }
    float3 N = cross(T, B);
    return float3x3(T, B, N);
}
