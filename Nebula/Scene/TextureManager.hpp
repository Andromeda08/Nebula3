#pragma once

#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace RHI
{
    class Descriptor;
    class Image;
    class VulkanRHI;
}

class TextureManager
{
public:
    static constexpr uint32_t sMaxTextureCount = 100;

    nbl_DISABLE_COPY(TextureManager);

    explicit TextureManager(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
        for (int32_t& value : mMetaData)
        {
            value = 0;
        }

        mMetaTexture = mRHI->createImage({
            .extent        = { sMaxTextureCount, 1 },
            .format        = vk::Format::eR32Sint,
            .usageFlags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .createSampler = true,
            .debugName     = "TextureMeta"
        });

        mMetaStaging = mRHI->createBuffer({
            .size      = sMaxTextureCount * sizeof(int32_t),
            .type      = RHI::BufferType::Staging,
            .debugName = "TextureMeta-Staging",
        });

        // TODO: loadTexture("missingTexture.png", 0);

        createDescriptor();
    }

    void update(const vk::CommandBuffer& commandBuffer);

private:
    void updateMetaTexture(const vk::CommandBuffer& commandBuffer)
    {
        mMetaStaging->setData(mMetaData.data(), sMaxTextureCount * sizeof(int32_t));
        // TODO: Buffer to Image copy
    }

    void createDescriptor()
    {
        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding()
                    .setBinding(0)
                    .setDescriptorCount(sMaxTextureCount)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setStageFlags(vk::ShaderStageFlagBits::eFragment),
                vk::DescriptorSetLayoutBinding()
                    .setBinding(1)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageImage)
                    .setStageFlags(vk::ShaderStageFlagBits::eFragment),
            },
            .setCount = gFramesInFlight,
            .debugName = "TextureDescriptor",
        });

        // Set all textures in the descriptor array to the missing texture.
        std::array<vk::DescriptorImageInfo, sMaxTextureCount> textureImageInfos;
        for (size_t i = 1; i < sMaxTextureCount; i++)
        {
            textureImageInfos[i] = vk::DescriptorImageInfo()
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(mTextures[0]->getImageView())
                .setSampler(mTextures[0]->getSampler());
        }

        const auto metaImageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(mMetaTexture->getImageView())
            .setSampler(mMetaTexture->getSampler());

        auto initialWrite = RHI::DescriptorWriteInfo()
            .writeCombinedImageSamplers(0, textureImageInfos.size(), textureImageInfos.data())
            .writeStorageImages(1, 1, &metaImageInfo);

    }

    SPtr<RHI::Descriptor>               mDescriptor;
    std::array<SPtr<RHI::Image>, 100>   mTextures;

    SPtr<RHI::Image>                    mMetaTexture;
    SPtr<RHI::Buffer>                   mMetaStaging;
    std::array<int32_t, 100>            mMetaData {};

    SPtr<RHI::VulkanRHI>                mRHI;
};
