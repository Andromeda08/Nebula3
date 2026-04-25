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
    int32_t   _pad0;
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
