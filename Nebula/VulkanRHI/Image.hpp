#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    // Immutable properties of a Vulkan Image
    struct ImageProperties
    {
        vk::Format                 format            = vk::Format::eR32G32B32A32Sfloat;
        vk::Extent2D               extent            = { 1280, 720 };
        vk::SampleCountFlagBits    sampleCount       = vk::SampleCountFlagBits::e1;
        vk::ImageSubresourceRange  subresourceRange  = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
        vk::ImageSubresourceLayers subresourceLayers = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
        bool                       isAliased         = false;
    };

    // Mutable state of a Vulkan Image
    struct ImageState
    {
        vk::ImageLayout         layout      = vk::ImageLayout::eUndefined;
        vk::AccessFlags2        accessFlags = vk::AccessFlagBits2::eNone;
        vk::PipelineStageFlags2 stageFlags  = vk::PipelineStageFlagBits2::eNone;
    };

    struct RHIImageCreateInfo
    {
        vk::Extent2D        extent          = { 1280, 720 };
        vk::Format          format          = vk::Format::eR32G32B32A32Sfloat;
        vk::ImageUsageFlags usageFlags      = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;;
        bool                createSampler   = false;
        bool                aliased         = false;
        std::string         debugName       = "Unknown Image";
    };

    struct ImageCreateInfo : public RHIImageCreateInfo
    {
        SPtr<Device> device;
    };

    struct SwapchainImageWrapperCreateInfo
    {
        vk::Image           image;
        vk::ImageView       imageView;
        uint32_t            imageIndex;
        SPtr<Device>        device;
        class Swapchain*    pSwapchain;
    };

    class Image
    {
    public:
        nbl_DISABLE_COPY(Image);
        nbl_CTOR_SHARED(Image);

        explicit Image(const SwapchainImageWrapperCreateInfo& createInfo);
        static SPtr<Image> createSwapchainImageWrapper(const SwapchainImageWrapperCreateInfo& createInfo);

        ~Image();

        void useAllocation(VmaAllocation allocation, const VmaAllocationInfo& allocationInfo);

        const vk::Image&        getImage()      const { return mImage; }
        const vk::ImageView&    getImageView()  const { return mImageView; }
        const vk::Sampler&      getSampler()    const { return mSampler; }
        const ImageProperties&  getProperties() const { return mProperties; }
        ImageState              getState()      const { return mState; }

    private:
        static ImageProperties makeProperties(const ImageCreateInfo& imageInfo);

        vk::Image               mImage;
        vk::ImageView           mImageView;
        vk::Sampler             mSampler;
        ImageState              mState;

        bool                    mHasMemory      = false;
        VmaAllocation           mAllocation     = {};
        VmaAllocationInfo       mAllocationInfo = {};

        SPtr<Device>            mDevice;

        const ImageProperties   mProperties;
        const std::string       mDebugName;

        const bool              mIsSwapchainImage    = false;
        const uint32_t          mSwapchainImageIndex = 0;
    };
}