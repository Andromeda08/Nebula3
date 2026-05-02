#pragma once

#include <array>
#include <expected>
#include <queue>
#include <stb_image.h>
#include <string>

#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace RHI
{
    class Buffer;
    class CommandList;
    class Descriptor;
    class Image;
    class VulkanRHI;
}

struct TextureManagerCreateInfo
{
    SPtr<RHI::VulkanRHI> rhi;
};

class TextureManager
{
    struct TextureLoadInfo
    {
        SPtr<RHI::Buffer> stagingBuffer;
        SPtr<RHI::Image>  textureImage;
        uint32_t          slot;
    };
public:
    static constexpr uint32_t sMaxTextureCount  = 1024;
    static constexpr uint32_t sMissingTextureId = 0;

    nbl_DISABLE_COPY(TextureManager);
    nbl_CTOR(TextureManager);

    ~TextureManager() = default;

    RHI::Image* getTexture(uint32_t slot) const noexcept;

    [[nodiscard]] int32_t createSampler(const vk::SamplerCreateInfo& createInfo) noexcept
    {
        try
        {
            const auto sampler = mRHI->getDevice()->getHandle().createSampler(createInfo);
            mSamplers.push_back(sampler);
        }
        catch (const vk::SystemError& error)
        {
            spdlog::warn("Failed to create Sampler: {}", error.what());
            return -1;
        }
        return static_cast<int32_t>(mSamplers.size()) - 1;
    }

    /**
     * Loads a Texture from the specified file into a given slot.
     * [Note!] The texture will be loaded immediately for now.
     */
    uint32_t loadTexture(const std::string& textureFile, const std::optional<uint32_t>& slot = std::nullopt) noexcept;

    uint32_t loadTextureFromMemory(const std::string& label, const stbi_uc* pixels, int32_t width, int32_t height,
        const std::optional<uint32_t>& slot = std::nullopt,
        const std::optional<vk::SamplerCreateInfo>& samplerInfo = std::nullopt,
        vk::Format format = vk::Format::eR8G8B8A8Srgb) noexcept;

    uint32_t loadTextureFromMemory(
        const std::string&               label,
        const stbi_uc*                   pixels,
        int32_t                          width,
        int32_t                          height,
        const std::optional<int32_t>&    samplerSlot = std::nullopt,
        const std::optional<vk::Format>& format   = std::nullopt,
        const std::optional<uint32_t>&   slot     = std::nullopt) noexcept;

    void update(const RHI::CommandList* commandList) const;

    [[nodiscard]] const SPtr<RHI::Descriptor>& getDescriptor() const noexcept
    {
        return mDescriptor;
    }

    void generateMipmaps(const std::vector<int32_t>& slots, const vk::Filter filter = vk::Filter::eLinear) const
    {
        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            for (const auto& slot : slots)
            {
                mTextures[slot]->generateMipmaps(commandList, filter);
            }
        });
    }

private:
    // Blocking texture load.
    void loadImmediately(
        const TextureLoadInfo&        textureLoadInfo,
        const std::optional<int32_t>& samplerSlot = std::nullopt) noexcept;

    // Update slot validity in metadata, mark as dirty if needed.
    void setSlot(uint32_t slot, bool hasTexture) noexcept;

    // If dirty, update the metadata texture.
    void updateMetaTexture(const RHI::CommandList* commandList) const;

    // =====================================
    // Constructor functions
    // =====================================
    void createMetadataResources();
    void createDescriptor();
    void writeInitialDescriptors() const;

    // =====================================
    // Utility functions
    // =====================================
    [[nodiscard]] uint32_t acquireNextSlot() noexcept;

    [[nodiscard]] static uint64_t getTextureSize(const int w, const int h, const int c = 4) noexcept
    {
        return static_cast<uint64_t>(w) * static_cast<uint64_t>(h) * c;
    }

    static constexpr auto sMissingTextureName = "missingTexture.png";

    // Samplers
    std::vector<vk::Sampler> mSamplers;

    // Textures
    std::array<SPtr<RHI::Image>, sMaxTextureCount> mTextures;

    // Free slots
    std::queue<uint32_t>                    mFreeSlots;

    // Meta Texture
    bool                                    mMetaIsDirty = false;
    SPtr<RHI::Image>                        mMetaTexture;
    SPtr<RHI::Buffer>                       mMetaStaging;
    std::array<int32_t, sMaxTextureCount>   mMetaData {};

    SPtr<RHI::Descriptor>                   mDescriptor;
    SPtr<RHI::VulkanRHI>                    mRHI;
};
