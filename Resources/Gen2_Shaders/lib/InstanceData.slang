#pragma once

struct GPUInstanceData
{
    float4x4    model;
    float4x4    modelInverse;
    float4x4    modelPrevious;
    float4      aabbMin;
    float4      aabbMax;
    uint64_t    blas;
    int         geometryIndex;
    int         materialIndex;
    int         objectId;
    int         emitterIndex;

    bool isEmitter() { return emitterIndex >= 0; }
};

uint getTranslatedInstanceIndex(uint index, uint* map)
{
    return map[index];
}
