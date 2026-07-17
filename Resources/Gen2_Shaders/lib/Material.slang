#pragma once

#include "Constants.slang"

struct MaterialSample
{
    float4 albedo;
    float3 normal;
    float  metallic;
    float  roughness;
};

struct GPUMaterialData
{
    float4  solidColor;
    int     textureIndex;
    int     normalMapIndex;
    int     metallicRoughnessMapIndex;
    float   metallicFactor;
    float   roughnessFactor;
    int     isEmissive;
    uint    rtHitGroup;

    uint    bsdfIndex;
    float4  diffuse_Albedo;
    float   dielectric_IntIoR;
    float   dielectric_ExtIoR;

    MaterialSample sample(
        Sampler2D[nbl::MAX_TEXTURES] uTex,
        RWTexture2D<int>             uValidity,
        float3                       wNormal,
        float4                       wTangent,
        float2                       uv)
    {
        MaterialSample s;

        s.albedo = solidColor;
        if (textureIndex >= 0)
        {
            s.albedo = select(uValidity.Load(textureIndex) == nbl::TEXTURE_VALID,
                uTex[textureIndex].Sample(uv),
                uTex[nbl::MISSING_TEXTURE_INDEX].Sample(uv));
        }

        s.normal = normalize(wNormal);
        if (normalMapIndex >= 0 && uValidity.Load(normalMapIndex) == nbl::TEXTURE_VALID)
        {
            var T = normalize(wTangent.xyz - dot(s.normal, wTangent.xyz) * s.normal);
            var B = cross(s.normal, T) * wTangent.w;
            var TBN = float3x3(T, B, s.normal);

            var read = uTex[normalMapIndex].Sample(uv).rgb;
            s.normal = normalize(mul(TBN, read * 2.0 - 1.0));
        }

        s.metallic  = metallicFactor;
        s.roughness = roughnessFactor;
        if (metallicRoughnessMapIndex >= 0 && uValidity.Load(metallicRoughnessMapIndex) == nbl::TEXTURE_VALID)
        {
            var read = uTex[metallicRoughnessMapIndex].Sample(uv);
            s.metallic  *= read.b;
            s.roughness *= read.g;
        }

        return s;
    }
};
