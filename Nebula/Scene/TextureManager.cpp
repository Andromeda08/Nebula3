#include "TextureManager.hpp"

#include <vector>
#include <stb_image.h>
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

TextureManager::TextureManager(const TextureManagerCreateInfo& createInfo)
: mRHI(createInfo.rhi)
{
    createMetadataResources();

    const auto result = loadTexture(sMissingTextureName, sMissingTextureId);

    createDescriptor();
}

std::expected<bool, std::string> TextureManager::loadTexture(const std::string& textureFile, const uint32_t slot, const bool deferred) noexcept
{
    if (slot >= mTextures.size())
    {
        return std::unexpected(std::format(
            "Invalid texture slot: {}. (valid: [0, {}])", slot, sMaxTextureCount - 1));
    }

    int width, height, channels;
    stbi_uc* pixels = stbi_load(
        Configuration::getTextureFilePath(textureFile).c_str(),
        &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        return std::unexpected(std::format("Failed to load texture: {}", textureFile));
    }

    const auto size = getTextureSize(width, height, channels);
    const auto stagingBuffer = mRHI->createBuffer({
        .size      = size,
        .type      = RHI::BufferType::Staging,
        .debugName = textureFile,
    });
    stagingBuffer->setData(pixels, size);
    stbi_image_free(pixels);

    const auto image = mRHI->createImage({
        .extent = vk::Extent2D()
            .setWidth(static_cast<uint32_t>(width))
            .setHeight(static_cast<uint32_t>(height)),
        .format = vk::Format::eR8G8B8A8Srgb,
        .usageFlags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        .createSampler = true,
        .aliased = false,
        .debugName = std::format("{}[slot={}]", textureFile, slot),
    });

    const auto loadInfo = TextureLoadInfo {
        .stagingBuffer = stagingBuffer,
        .textureImage  = image,
        .slot          = slot
    };

    if (!deferred)
    {
        loadImmediately(loadInfo);
    }
    else
    {
        loadDeferred(loadInfo);
    }

    return true;
}

void TextureManager::update(const RHI::CommandList* commandList)
{
    clearUploadQueue();
    if (!mQueuedLoads.empty())
    {
        uploadQueuedTextures(commandList);
    }
    if (mMetaIsDirty)
    {
        updateMetaTexture(commandList);
    }
}

void TextureManager::loadImmediately(const TextureLoadInfo& textureLoadInfo) noexcept
{
    const auto [stagingBuffer, textureImage, slot] = textureLoadInfo;
    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
        RHI::Barrier().addImageBarrier({
            .dstUsage = RHI::ImageUsage::TransferDst,
            .image = textureImage,
        }).insert(commandList);
        commandList->copyBufferToImage({
            .pSrcBuffer = stagingBuffer.get(),
            .pDstImage  = textureImage.get(),
        });
        RHI::Barrier().addImageBarrier({
            .dstUsage = RHI::ImageUsage::ShaderReadOnly,
            .image = textureImage,
        }).insert(commandList);

        updateMetaTexture(commandList);
    });

    mTextures[slot] = textureImage;
    setSlot(slot, true);

    const auto textureImageInfo = vk::DescriptorImageInfo()
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setImageView(textureImage->getImageView())
        .setSampler(textureImage->getSampler());

    auto write = RHI::DescriptorWriteInfo()
        .writeCombinedImageSamplers(0, 1, &textureImageInfo);

    for (auto i = 0; i < mDescriptor->getSetCount(); i++)
    {
        write.setSetIndex(i);
        mDescriptor->write(write);
    }
}

void TextureManager::loadDeferred(const TextureLoadInfo& textureLoadInfo)
{
    mQueuedLoads.push_back(textureLoadInfo);
}

void TextureManager::setSlot(const uint32_t slot, const bool hasTexture) noexcept
{
    assert(slot < mMetaData.size());
    const auto asNumber = hasTexture ? 1 : 0;
    if (mMetaData[slot] == asNumber)
    {
        return;
    }

    mMetaData[slot] = asNumber;
    mMetaIsDirty = true;
}

void TextureManager::updateMetaTexture(const RHI::CommandList* commandList) const
{
    mMetaStaging->setData(mMetaData.data(), sMaxTextureCount * sizeof(int32_t));

    RHI::Barrier().addImageBarrier({
        .dstUsage = RHI::ImageUsage::TransferDst,
        .image = mMetaTexture,
    }).insert(commandList);
    commandList->copyBufferToImage({
        .pSrcBuffer = mMetaStaging.get(),
        .pDstImage  = mMetaTexture.get(),
    });
    RHI::Barrier().addImageBarrier({
        .dstUsage = RHI::ImageUsage::ShaderReadOnly,
        .image = mMetaTexture,
    }).insert(commandList);
}

void TextureManager::uploadQueuedTextures(const RHI::CommandList* commandList)
{
    // Prepare
    auto toTransferDstBarrier = RHI::Barrier();
    auto toShaderReadOnlyBarrier = RHI::Barrier();
    std::vector<vk::DescriptorImageInfo> textureImageInfos;

    for (const auto& loadInfo : mQueuedLoads)
    {
        toTransferDstBarrier.addImageBarrier({
            .dstUsage = RHI::ImageUsage::TransferDst,
            .image    = loadInfo.textureImage,
        });
        toShaderReadOnlyBarrier.addImageBarrier({
            .dstUsage = RHI::ImageUsage::ShaderReadOnly,
            .image    = loadInfo.textureImage
        });
        textureImageInfos.push_back(vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(loadInfo.textureImage->getImageView())
            .setSampler(loadInfo.textureImage->getSampler()));

        mTextures[loadInfo.slot] = loadInfo.textureImage;
        setSlot(loadInfo.slot, true);

        mLoadInfoDeletionQueue.insert(loadInfo.slot);
    }

    // Record commands
    toTransferDstBarrier.insert(commandList);
    for (const auto& loadInfo : mQueuedLoads)
    {
        commandList->copyBufferToImage({
            .pSrcBuffer = loadInfo.stagingBuffer.get(),
            .pDstImage  = loadInfo.textureImage.get(),
        });
    }
    toShaderReadOnlyBarrier.insert(commandList);

    updateMetaTexture(commandList);

    auto write = RHI::DescriptorWriteInfo()
        .writeCombinedImageSamplers(0, textureImageInfos.size(), textureImageInfos.data());

    for (auto i = 0; i < mDescriptor->getSetCount(); i++)
    {
        write.setSetIndex(i);
        mDescriptor->write(write);
    }
}

void TextureManager::clearUploadQueue()
{
    mQueuedLoads = mQueuedLoads
        | std::views::filter([&](const auto& loadInfo) -> bool {
            return !mLoadInfoDeletionQueue.contains(loadInfo.slot);
        })
        | std::ranges::to<std::vector<TextureLoadInfo>>();
    mLoadInfoDeletionQueue.clear();
}

void TextureManager::createMetadataResources()
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
}

void TextureManager::createDescriptor()
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

    for (auto i = 0; i < mDescriptor->getSetCount(); i++)
    {
        initialWrite.setSetIndex(i);
        mDescriptor->write(initialWrite);
    }
}
