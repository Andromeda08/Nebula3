#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/ImageTraits.hpp"

namespace RHI
{
    struct RHIImage3DCreateInfo
    {
        vk::Extent3D            extent      = { 400, 400, 400 };
        vk::Format              format      = vk::Format::eR32G32B32A32Sfloat;
        vk::ImageUsageFlags     usageFlags  = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
        vk::SampleCountFlagBits samples     = vk::SampleCountFlagBits::e1;
        std::string             debugName   = "Unknown Image3D";
    };

    struct Image3DCreateInfo : public RHIImage3DCreateInfo
    {
        SPtr<Device> device;
    };

    class Image3D
    {
    public:
        nbl_DISABLE_COPY(Image3D);
        nbl_CTOR_SHARED(Image3D);

        ~Image3D() = default;

        /**
         * Create an ImageMemoryBarrier to the dstState.
         * @param dstUsage
         * @param update Update the internally tracked state
         */
        vk::ImageMemoryBarrier2 getBarrier(const ImageUsage& dstUsage, const bool update = true) noexcept
        {
            const auto dstState = getImageState(dstUsage);
            const auto barrier = makeImageMemoryBarrier(mState, dstState)
                .setImage(mImage)
                .setSubresourceRange({ mProperties.aspectFlags, 0, 1, 0, 1 });

            if (update)
            {
                mState = dstState;
            }

            return barrier;
        }

        vk::Extent3D         getExtent3D()   const { return mExtent3D; }
        const vk::Image&     getImage()      const { return mImage; }
        const vk::ImageView& getImageView()  const { return mImageView; }
        ImageState           getState()      const { return mState; }

    private:
        vk::Extent3D            mExtent3D;
        vk::Image               mImage;
        vk::ImageView           mImageView;
        ImageState              mState;

        VmaAllocation           mAllocation     = {};
        VmaAllocationInfo       mAllocationInfo = {};

        SPtr<Device>            mDevice;
        const ImageProperties   mProperties;
        const std::string       mDebugName;
    };

    inline Image3D::Image3D(const Image3DCreateInfo& createInfo)
    : mExtent3D(createInfo.extent)
    , mDevice(createInfo.device)
    , mProperties({ createInfo.format, {}, getImageAspectFlags(createInfo.format)})
    , mDebugName(createInfo.debugName)
    {
        /* Create Image & Allocate Memory */ {
            auto imageCreateInfo = vk::ImageCreateInfo()
                .setFormat(mProperties.format)
                .setExtent(mExtent3D)
                .setSamples(mProperties.sampleCount)
                .setUsage(createInfo.usageFlags)
                .setTiling(vk::ImageTiling::eOptimal)
                .setArrayLayers(1)
                .setMipLevels(1)
                .setImageType(vk::ImageType::e3D)
                .setSharingMode(vk::SharingMode::eExclusive)
                .setInitialLayout(vk::ImageLayout::eUndefined);

            const auto* pImageInfo = reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo);
            auto* pImage = reinterpret_cast<VkImage*>(&mImage);
            VmaAllocationCreateInfo allocationInfo {};
            allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
            vmaCreateImage(mDevice->getAllocator(), pImageInfo, &allocationInfo, pImage, &mAllocation, &mAllocationInfo);

            mDevice->nameObject<vk::Image>({
                .debugName = mDebugName,
                .handle = mImage,
            });
        }

        /* ImageView */ {
            const auto viewCreateInfo = vk::ImageViewCreateInfo()
                .setFormat(mProperties.format)
                .setImage(mImage)
                .setSubresourceRange({ mProperties.aspectFlags, 0, 1, 0, 1 })
                .setViewType(vk::ImageViewType::e3D);

            mImageView = mDevice->getHandle().createImageView(viewCreateInfo);

            mDevice->nameObject<vk::ImageView>({
                .debugName = std::format("{} View", mDebugName),
                .handle    = mImageView,
            });
        }
    }
}
