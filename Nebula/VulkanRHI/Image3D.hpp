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

        ~Image3D();

        /**
         * Create an ImageMemoryBarrier to the dstState.
         * @param dstUsage
         * @param update Update the internally tracked state
         */
        vk::ImageMemoryBarrier2 getBarrier(const ImageUsage& dstUsage, const bool update = true) noexcept;

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
}
