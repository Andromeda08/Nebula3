#pragma once

#include <array>
#include <expected>
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

    struct BatchUpload
    {
        struct BatchItem
        {
            SPtr<RHI::Image>  textureImage;
            uint64_t          offset;
            uint32_t          slot;
        };

        BatchUpload& addTexture(const std::string& label, const stbi_uc* pixels, const int32_t w, const int32_t h, const uint32_t slot) noexcept
        {
            const auto size = getTextureSize(w, h);

            const auto offset = mStagingSize;
            const auto requiredSize = mStagingSize + size;
            if (requiredSize > mStagingCapacity)
            {
                mStagingCapacity = std::max(requiredSize, mStagingCapacity * 2);

                auto newStaging = mRHI->createBuffer({
                    .size  = mStagingCapacity,
                    .type  = RHI::BufferType::Staging,
                    .label = "TextureBatchStaging"
                });

                if (mStaging)
                {
                    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
                       const auto region = vk::BufferCopy2()
                            .setSrcOffset(0)
                            .setDstOffset(0)
                            .setSize(mStaging->getSize());
                       const auto copyInfo = vk::CopyBufferInfo2()
                           .setSrcBuffer(mStaging->getHandle())
                           .setDstBuffer(newStaging->getHandle())
                           .setRegions(region);
                       pCommandList->getHandle().copyBuffer2(copyInfo);
                    });
                }

                mStaging = std::move(newStaging);
            }

            mStagingSize = requiredSize;

            mStaging->setData(pixels, size, offset);

            mItems.push_back({
                .textureImage = mRHI->createImage({
                    .extent = vk::Extent2D()
                        .setWidth(static_cast<uint32_t>(w))
                        .setHeight(static_cast<uint32_t>(h)),
                    .format = vk::Format::eR8G8B8A8Srgb,
                    .usageFlags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                    .createSampler = true,
                    .aliased = false,
                    .debugName = std::format("{}[slot={}]", label, slot),
                }),
                .offset = offset,
                .slot = slot,
            });

            return *this;
        }

        std::vector<BatchItem>  mItems           = {};
        uint64_t                mStagingSize     = 0;
        uint64_t                mStagingCapacity = 0;
        SPtr<RHI::Buffer>       mStaging         = nullptr;
        SPtr<RHI::VulkanRHI>    mRHI;
    };
public:
    static constexpr uint32_t sMaxTextureCount  = 1024;
    static constexpr uint32_t sMissingTextureId = 0;

    nbl_DISABLE_COPY(TextureManager);
    nbl_CTOR(TextureManager);

    ~TextureManager() = default;

    RHI::Image* getTexture(uint32_t slot) const noexcept;

    /**
     * Loads a Texture from the specified file into a given slot.
     * [Note!] The texture will be loaded immediately for now.
     * @returns If successfully loaded true, otherwise an error message.
     */
    std::expected<bool, std::string> loadTexture(const std::string& textureFile, uint32_t slot) noexcept;

    void loadTextureFromMemory(const std::string& label, const stbi_uc* pixels, int32_t width, int32_t height, uint32_t slot) noexcept;

    BatchUpload createBatchUpload() noexcept;

    void loadTextureBatch(const BatchUpload& batch) noexcept;

    void update(const RHI::CommandList* commandList) const;

    [[nodiscard]] const SPtr<RHI::Descriptor>& getDescriptor() const noexcept
    {
        return mDescriptor;
    }

private:
    // Blocking texture load.
    void loadImmediately(const TextureLoadInfo& textureLoadInfo) noexcept;

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
    [[nodiscard]] static uint64_t getTextureSize(const int w, const int h, const int c = 4) noexcept
    {
        return static_cast<uint64_t>(w) * static_cast<uint64_t>(h) * c;
    }

    static constexpr auto sMissingTextureName = "missingTexture.png";

    // Textures
    std::array<SPtr<RHI::Image>, sMaxTextureCount> mTextures;

    // Meta Texture
    bool                                    mMetaIsDirty = false;
    SPtr<RHI::Image>                        mMetaTexture;
    SPtr<RHI::Buffer>                       mMetaStaging;
    std::array<int32_t, sMaxTextureCount>   mMetaData {};

    SPtr<RHI::Descriptor>                   mDescriptor;
    SPtr<RHI::VulkanRHI>                    mRHI;
};
