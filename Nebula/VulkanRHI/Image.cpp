#include "Image.hpp"

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
                .sampleCount = createInfo.samples,
            };
        }
    }

    Image::Image(const ImageCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mProperties(detail::makeImageProperties(createInfo))
    , mDebugName(createInfo.debugName)
    {
        /**
         * Create Image
         */
        auto imageCreateInfo = vk::ImageCreateInfo()
            .setFormat(mProperties.format)
            .setExtent({ mProperties.extent.width, mProperties.extent.height, 1 })
            .setSamples(mProperties.sampleCount)
            .setUsage(createInfo.usageFlags)
            .setTiling(vk::ImageTiling::eOptimal)
            .setArrayLayers(1)
            .setMipLevels(1)
            .setImageType(vk::ImageType::e2D)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        if (createInfo.aliased)
        {
            imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eAlias);
        }

        const auto* pImageInfo = reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo);
        auto* pImage = reinterpret_cast<VkImage*>(&mImage);

        if (!createInfo.aliased)
        {
            VmaAllocationCreateInfo allocationInfo {};
            allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
            vmaCreateImage(mDevice->getAllocator(), pImageInfo, &allocationInfo, pImage, &mAllocation, &mAllocationInfo);
            mHasMemory = true;
        }

        mDevice->nameObject<vk::Image>({
            .debugName = mDebugName,
            .handle = mImage,
        });

        /**
         * Create ImageView
         */
        const auto viewCreateInfo = vk::ImageViewCreateInfo()
            .setFormat(mProperties.format)
            .setImage(mImage)
            .setSubresourceRange({ mProperties.aspectFlags, 0, mProperties.levelCount, 0, 1 })
            .setViewType(vk::ImageViewType::e2D);

        mImageView = mDevice->getHandle().createImageView(viewCreateInfo);

        mDevice->nameObject<vk::ImageView>({
            .debugName = std::format("{} View", mDebugName),
            .handle    = mImageView,
        });

        /**
         * Create Sampler
         */
        if (createInfo.createSampler)
        {
            constexpr auto samplerCreateInfo = vk::SamplerCreateInfo()
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setAnisotropyEnable(true)
                .setMaxAnisotropy(1.0)
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
                .debugName = std::format("{} Sampler", mDebugName),
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

        vmaDestroyImage(mDevice->getAllocator(), mImage, mAllocation);
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
