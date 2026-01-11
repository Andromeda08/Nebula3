#include "Image3D.hpp"

namespace RHI
{
    Image3D::Image3D(const Image3DCreateInfo& createInfo)
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

    Image3D::~Image3D()
    {
        mDevice->getHandle().destroyImageView(mImageView);
        vmaDestroyImage(mDevice->getAllocator(), mImage, mAllocation);
    }

    vk::ImageMemoryBarrier2 Image3D::getBarrier(const ImageUsage& dstUsage, const bool update) noexcept
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
}
