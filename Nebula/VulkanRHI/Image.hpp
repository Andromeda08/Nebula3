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

        [[nodiscard]] vk::Extent3D getExtent3D() const
        {
                return { extent.width, extent.height, 1 };
        }
    };

    // Mutable state of a Vulkan Image
    struct ImageState
    {
        vk::ImageLayout         layout      = vk::ImageLayout::eUndefined;
        vk::AccessFlags2        accessFlags = vk::AccessFlagBits2::eNone;
        vk::PipelineStageFlags2 stageFlags  = vk::PipelineStageFlagBits2::eNone;
    };

    // =====================================
    // ImageUsage Enum and Utilities
    // =====================================

    enum class ImageUsage
    {
        Undefined,
        ColorAttachment,
        Clear,
        General,
        ShaderReadOnly,
        StorageImage,
        TransferSrc,
        TransferDst,
        PresentSrc,
    };

    constexpr ImageState getImageStateForUsage(const ImageUsage usage)
    {
        using enum ImageUsage;
        switch (usage)
        {
            case Undefined: {
                return {
                    .layout      = vk::ImageLayout::eUndefined,
                    .accessFlags = vk::AccessFlagBits2::eNone,
                    .stageFlags  = vk::PipelineStageFlagBits2::eNone,
                };
            }
            case ColorAttachment: {
                return {
                    .layout      = vk::ImageLayout::eColorAttachmentOptimal,
                    .accessFlags = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite,
                    .stageFlags  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                };
            }
            case Clear:  {
                return {
                    .layout      = vk::ImageLayout::eTransferDstOptimal,
                    .accessFlags = vk::AccessFlagBits2::eTransferWrite,
                    .stageFlags  = vk::PipelineStageFlagBits2::eClear,
                };
            }
            case General: {
                return {
                    .layout      = vk::ImageLayout::eGeneral,
                    .accessFlags = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                    .stageFlags  = vk::PipelineStageFlagBits2::eAllCommands,
                };
            }
            case ShaderReadOnly: {
                return {
                    .layout      = vk::ImageLayout::eShaderReadOnlyOptimal,
                    .accessFlags = vk::AccessFlagBits2::eShaderRead,
                    .stageFlags  = vk::PipelineStageFlagBits2::eAllCommands,
                };
            }
            case StorageImage: {
                return {
                    .layout      = vk::ImageLayout::eGeneral,
                    .accessFlags = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                    .stageFlags  = vk::PipelineStageFlagBits2::eAllCommands,
                };
            }
            case TransferSrc: {
                return {
                    .layout      = vk::ImageLayout::eTransferSrcOptimal,
                    .accessFlags = vk::AccessFlagBits2::eTransferRead,
                    .stageFlags  = vk::PipelineStageFlagBits2::eTransfer,
                };
            }
            case TransferDst: {
                return {
                    .layout      = vk::ImageLayout::eTransferDstOptimal,
                    .accessFlags = vk::AccessFlagBits2::eTransferWrite,
                    .stageFlags  = vk::PipelineStageFlagBits2::eTransfer,
                };
            }
            case PresentSrc: {
                return {
                    .layout      = vk::ImageLayout::ePresentSrcKHR,
                    .accessFlags = vk::AccessFlagBits2::eNone,
                    .stageFlags  = vk::PipelineStageFlagBits2::eNone,
                };
            }
            default: {
                assert(false);
                return {};
            }
        }
    }

    constexpr vk::ImageUsageFlags getImageUsageFlags(const ImageUsage usage)
    {
        using enum ImageUsage;
        vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        switch (usage)
        {
            case ColorAttachment: {
                usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
                break;
            }
            case General: {
                usageFlags |= vk::ImageUsageFlagBits::eStorage
                    | vk::ImageUsageFlagBits::eColorAttachment
                    | vk::ImageUsageFlagBits::eSampled;
                break;
            }
            case ShaderReadOnly: {
                usageFlags |= vk::ImageUsageFlagBits::eSampled;
                break;
            }
            case StorageImage: {
                usageFlags |= vk::ImageUsageFlagBits::eStorage;
                break;
            }
            case TransferSrc: {
                usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
                break;
            }
            case TransferDst: {
                usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
                break;
            }

            // Usage modes not related to usage flags.
            case PresentSrc:
            case Clear:
            default: {
                usageFlags = {};
            }
        }
        return usageFlags;
    }

    // =====================================
    // Image Class
    // =====================================

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

        void updateState(const ImageState& imageState)
        {
            mState = imageState;
        }

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