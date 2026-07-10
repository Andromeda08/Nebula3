#pragma once

#include <glm/glm.hpp>
#include "../SlotPool.hpp"

using TextureHandle = int32_t;

struct GPUMaterialData
{
    glm::vec4 solidColor;
    int32_t   textureIndex;
    int32_t   normalMapIndex;
    int32_t   metallicRoughnessMapIndex;
    float     metallicFactor;
    float     roughnessFactor;
    int32_t   isEmissive;
    uint32_t  rtHitGroup;

    uint32_t  bsdfIndex;
    glm::vec4 diffuse_Albedo;
    float     dielectric_IntIoR;
    float     dielectric_ExtIoR;
};

struct MaterialData
{
    static constexpr auto sDefaultColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

    // Material Parameters
    glm::vec4       solidColor              = sDefaultColor;
    TextureHandle   hTexture                = -1;
    TextureHandle   hNormalMap              = -1;
    TextureHandle   hMetallicRoughnessMap   = -1;
    float           pMetallicFactor         = 1.0f;
    float           pRoughnessFactor        = 1.0f;
    bool            pIsEmissive             = false;

    // Ray Tracing
    uint32_t        rtHitGroup              = 0;

    // Path Tracer Params
    uint32_t        bsdfIndex               = 0;
    glm::vec4       diffuse_albedo          = glm::vec4(1.0f);
    float           dielectric_IntIoR       = 1.5046f;
    float           dielectric_ExtIoR       = 1.000277f;

    [[nodiscard]] GPUMaterialData toGPU() const noexcept
    {
        return {
            .solidColor                 = solidColor,
            .textureIndex               = hTexture,
            .normalMapIndex             = hNormalMap,
            .metallicRoughnessMapIndex  = hMetallicRoughnessMap,
            .metallicFactor             = pMetallicFactor,
            .roughnessFactor            = pRoughnessFactor,
            .isEmissive                 = pIsEmissive ? 1 : 0,
            .rtHitGroup                 = rtHitGroup,
            .bsdfIndex                  = bsdfIndex,
            .diffuse_Albedo             = diffuse_albedo,
            .dielectric_IntIoR          = dielectric_IntIoR,
            .dielectric_ExtIoR          = dielectric_ExtIoR,
        };
    }
};

using MaterialPool = Pool<MaterialData, GPUMaterialData>;

namespace nbl
{
    class MaterialSystem : public Pool<MaterialData, GPUMaterialData>
    {
    public:
        explicit MaterialSystem(const SPtr<RHI::VulkanRHI>& rhi, const uint32_t capacity = 4096)
        : Pool(rhi, "MaterialSystem", capacity)
        {
        }

        void onUpdate(const RHI::CommandList* commandList)
        {
            flush(commandList);
        }
    };
}
