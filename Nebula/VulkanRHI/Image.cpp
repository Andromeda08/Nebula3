#include "Image.hpp"

#include "Commands/CommandList.hpp"

namespace RHI
{
    namespace detail
    {
        // Define and compute the immutable properties an image based on the given parameters.
        [[nodiscard]] static ImageProperties makeImageProperties(const ImageCreateInfo& createInfo) noexcept
        {
            return {
                .format      = createInfo.format,
                .extent      = createInfo.extent,
                .aspectFlags = getImageAspectFlags(createInfo.format),
                .levelCount  = (createInfo.mipmapping) ? getMipLevels(createInfo.extent) : 1,
                .layerCount  = createInfo.cubeMap ? 6u : 1u,
                .sampleCount = createInfo.samples,
                .usageFlags  = createInfo.usageFlags,
            };
        }
    }

    Image::Image(const ImageCreateInfo& createInfo)
    : Resource(mDevice)
    , mProperties(detail::makeImageProperties(createInfo))
    {
        setLabel(createInfo.debugName);

        /**
         * Create Image
         */
        auto imageCreateInfo = vk::ImageCreateInfo()
            .setFormat(mProperties.format)
            .setExtent({ mProperties.extent.width, mProperties.extent.height, 1 })
            .setSamples(mProperties.sampleCount)
            .setUsage(createInfo.usageFlags)
            .setTiling(vk::ImageTiling::eOptimal)
            .setArrayLayers(createInfo.cubeMap ? 6 : 1)
            .setMipLevels(mProperties.levelCount)
            .setImageType(vk::ImageType::e2D)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        if (createInfo.cubeMap)
        {
            imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
        }
        if (createInfo.aliased)
        {
            imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eAlias);
        }

        if (!createInfo.aliased)
        {
            const ImageMemoryAllocationInfo allocInfo = {
                .pHandle   = &mImage,
                .imageInfo = imageCreateInfo,
            };
            const auto allocation = mDevice->allocateImage(allocInfo);
            setAllocation(allocation);
        }

        mDevice->nameObject<vk::Image>({
            .debugName = mLabel,
            .handle    = mImage,
        });

        /**
         * Create ImageView
         */
        const auto viewCreateInfo = vk::ImageViewCreateInfo()
            .setFormat(mProperties.format)
            .setImage(mImage)
            .setSubresourceRange({ mProperties.aspectFlags, 0, 1, 0, static_cast<uint32_t>(createInfo.cubeMap ? 6 : 1) })
            .setViewType(createInfo.cubeMap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D);

        mImageView = mDevice->getHandle().createImageView(viewCreateInfo);

        mDevice->nameObject<vk::ImageView>({
            .debugName = std::format("{} View", mLabel),
            .handle    = mImageView,
        });

        if (createInfo.mipmapping)
        {
            mMipViews.resize(mProperties.levelCount);
            for (uint32_t i = 0; i < mProperties.levelCount; i++)
            {
                const auto mipViewInfo = vk::ImageViewCreateInfo()
                    .setFormat(mProperties.format)
                    .setImage(mImage)
                    .setSubresourceRange({ mProperties.aspectFlags, i, 1, 0, 1 })
                    .setViewType(createInfo.cubeMap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D);
                mMipViews[i] = mDevice->getHandle().createImageView(mipViewInfo);

                mDevice->nameObject<vk::ImageView>({
                    .debugName = std::format("{} View [mip={}]", mLabel, i),
                    .handle    = mMipViews[i],
                });
            }
        }

        /**
         * Create Sampler
         */
        //if (createInfo.createSampler)
        {
            auto samplerCreateInfo = vk::SamplerCreateInfo();

            if (createInfo.samplerInfo.has_value())
            {
                samplerCreateInfo = createInfo.samplerInfo.value();
            }
            else
            {
                samplerCreateInfo
                    .setMagFilter(vk::Filter::eLinear)
                    .setMinFilter(vk::Filter::eLinear)
                    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeW(vk::SamplerAddressMode::eRepeat);
            }

            samplerCreateInfo
                .setAnisotropyEnable(true)
                .setMaxAnisotropy(8.0)
                .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(false)
                .setCompareEnable(false)
                .setCompareOp(vk::CompareOp::eAlways)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
                .setMaxLod(0.0f);

            mSampler = mDevice->getHandle().createSampler(samplerCreateInfo);

            mDevice->nameObject<vk::Sampler>({
                .debugName = std::format("{} Sampler", mLabel),
                .handle = mSampler,
            });
        }
    }

    Image::~Image()
    {
        if (mSampler)
        {
            mDevice->getHandle().destroySampler(mSampler);
        }

        mDevice->getHandle().destroyImageView(mImageView);

        mAllocation->free();
    }

    const vk::ImageView& Image::getMipView(const size_t i) const noexcept
    {
        return mMipViews[i];
    }

    void Image::generateMipmaps(const CommandList* commandList, const vk::Filter filter)
    {
        const auto commandBuffer = commandList->getHandle();
        const auto layerCount    = mProperties.layerCount;

        /* Transition base mip level to TransferSrc */
        {

            const auto barrier_Undef_TDst = vk::ImageMemoryBarrier2()
                .setImage(mImage)
                .setSubresourceRange({ mProperties.aspectFlags, 0, 1, 0, layerCount })
                .setOldLayout(mState.layout)
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcAccessMask(mState.accessMask)
                .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
                .setSrcStageMask(mState.stageMask)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBlit);
            const auto dependencyInfo = vk::DependencyInfo().setImageMemoryBarriers(barrier_Undef_TDst);
            commandBuffer.pipelineBarrier2(dependencyInfo);
        }

        // Blit mip levels i-1 to i
        const auto w = static_cast<int32_t>(mProperties.extent.width);
        const auto h = static_cast<int32_t>(mProperties.extent.height);
        for (uint32_t i = 1; i < mProperties.levelCount; i++)
        {
            // current mip level to transfer dst
            const auto barrier_Undef_TDst = vk::ImageMemoryBarrier2()
                .setImage(mImage)
                .setSubresourceRange({ mProperties.aspectFlags, i, 1, 0, layerCount })
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBlit);
            const auto dependencyInfo_Pre = vk::DependencyInfo().setImageMemoryBarriers(barrier_Undef_TDst);

            // Blit
            const auto imageBlit = vk::ImageBlit2()
                .setSrcSubresource({ mProperties.aspectFlags, static_cast<uint32_t>(i - 1), 0, layerCount})
                .setSrcOffsets({
                    vk::Offset3D { 0, 0, 0 },
                    vk::Offset3D { w >> (i - 1), h >> (i - 1), 1 },
                })
                .setDstSubresource({ mProperties.aspectFlags, static_cast<uint32_t>(i), 0, layerCount})
                .setDstOffsets({
                    vk::Offset3D { 0, 0, 0 },
                    vk::Offset3D { w >> i, h >> i,  1 },
                });

            const auto blitImageInfo = vk::BlitImageInfo2()
                .setSrcImage(mImage)
                .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setDstImage(mImage)
                .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
                .setFilter(filter)
                .setRegions(imageBlit);

            // current mip level to transfer src
            const auto barrier_TDst_TSrc = vk::ImageMemoryBarrier2()
                .setImage(mImage)
                .setSubresourceRange({ mProperties.aspectFlags, i, 1, 0, layerCount })
                .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eBlit)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBlit);
            const auto dependencyInfo_Post = vk::DependencyInfo().setImageMemoryBarriers(barrier_TDst_TSrc);

            commandBuffer.pipelineBarrier2(dependencyInfo_Pre);
            commandBuffer.blitImage2(blitImageInfo);
            commandBuffer.pipelineBarrier2(dependencyInfo_Post);
        }

        // Transition all mip levels to General layout
        const auto barrier_TSrc_General = vk::ImageMemoryBarrier2()
            .setImage(mImage)
            .setSubresourceRange({ mProperties.aspectFlags, 0, mProperties.levelCount, 0, layerCount })
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eBlit)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
        const auto dependencyInfo_End = vk::DependencyInfo().setImageMemoryBarriers(barrier_TSrc_General);
        commandBuffer.pipelineBarrier2(dependencyInfo_End);

        mState.accessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
        mState.stageMask  = vk::PipelineStageFlagBits2::eAllGraphics;
        mState.layout     = vk::ImageLayout::eGeneral;
    }

    void Image::useAllocation(VmaAllocation allocation, const VmaAllocationInfo& allocationInfo)
    {
        if (!mHasMemory)
        {
            mAllocation = allocation;
            mAllocationInfo = allocationInfo;
            vmaBindImageMemory(mDevice->getAllocator(), mAllocation, mImage);
        }
    }
}
