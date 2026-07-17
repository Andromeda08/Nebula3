#pragma once

namespace nbl
{
    static const uint MAX_LIGHTS = 64;

    // Light Type Enum
    static const int LIGHT_POINT       = 0;
    static const int LIGHT_DIRECTIONAL = 1;
}

struct GPULightData
{
    float3  vector;
    int     lightType;
    float3  color;
    float   intensity;
    int     isEnabled;
    int     castsShadows;
    float   radius;
};

struct AreaEmitter
{
    uint   instanceIndex;
    int    geometryIndex;
    uint   cdfOffset;
    uint   triCount;
    float  totalWeight;
    float3 radiance;
};
