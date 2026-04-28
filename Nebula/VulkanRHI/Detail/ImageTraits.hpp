#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_format_traits.hpp>

namespace RHI
{
    // =======================================
    // Image : Utility Functions
    // =======================================

    // Get the aspect flags for a specific format.
    [[nodiscard]] constexpr vk::ImageAspectFlags getImageAspectFlags(const vk::Format format) noexcept
    {
        vk::ImageAspectFlags aspectFlags = {};

        if (vk::isColor(format))
        {
            aspectFlags = vk::ImageAspectFlagBits::eColor;
        }
        if (vk::hasDepthComponent(format))
        {
            aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        }
        if (vk::hasStencilComponent(format))
        {
            aspectFlags |= vk::ImageAspectFlagBits::eStencil;
        }

        return aspectFlags;
    }

    // Get the number of mip levels for an extent.
    [[nodiscard]] inline uint32_t getMipLevels(const vk::Extent2D& extent) noexcept
    {
        return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    }

    // =======================================
    // Image : Usage Modes and Mutable State
    // =======================================

    // Common Image usage modes.
    enum class ImageUsage
    {
        Undefined,
        ColorAttachment,
        DepthAttachment,
        Clear,
        General,
        ShaderReadOnly,
        StorageImage,
        TransferSrc,
        TransferDst,
        PresentSrc,
    };

    // Tracking the mutable state of an Image.
    struct ImageState
    {
        vk::ImageLayout         layout     = vk::ImageLayout::eUndefined;
        vk::AccessFlags2        accessMask = vk::AccessFlagBits2::eNone;
        vk::PipelineStageFlags2 stageMask  = vk::PipelineStageFlagBits2::eNone;
    };

    /**
     * For a specific usage, get the dst ImageState for memory barriers.
     */
    [[nodiscard]] constexpr ImageState getImageState(const ImageUsage usage) noexcept
    {
        // TODO: Propagate this somehow from RHI and use eGeneral when the feature is available.
        constexpr bool hasUnifiedLayouts = false;

        using L = vk::ImageLayout;
        using A = vk::AccessFlagBits2;
        using S = vk::PipelineStageFlagBits2;

        using enum ImageUsage;
        switch (usage)
        {
            case Undefined: {
                return { L::eUndefined, A::eNone, S::eNone };
            }
            case ColorAttachment: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eColorAttachmentOptimal,
                    .accessMask = A::eColorAttachmentRead | A::eColorAttachmentWrite,
                    .stageMask  = S::eColorAttachmentOutput,
                };
            }
            case DepthAttachment: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eDepthStencilAttachmentOptimal,
                    .accessMask = A::eDepthStencilAttachmentRead | A::eDepthStencilAttachmentWrite,
                    .stageMask  = S::eColorAttachmentOutput | S::eEarlyFragmentTests | S::eLateFragmentTests,
                };
            }
            case Clear: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eTransferDstOptimal,
                    .accessMask = A::eTransferWrite,
                    .stageMask = S::eClear,
                };
            }
            case General: {
                return {
                    .layout     = L::eGeneral,
                    .accessMask = A::eMemoryRead | A::eMemoryWrite,
                    .stageMask = S::eAllCommands,
                };
            }
            case ShaderReadOnly: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eShaderReadOnlyOptimal,
                    .accessMask = A::eShaderRead | A::eShaderStorageRead | A::eShaderSampledRead,
                    .stageMask  = S::eAllCommands,
                };
            }
            case StorageImage: {
                return {
                    .layout     = L::eGeneral,
                    .accessMask = A::eShaderRead | A::eShaderWrite | A::eShaderStorageRead | A::eShaderStorageWrite,
                    .stageMask = S::eAllCommands,
                };
            }
            case TransferSrc: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eTransferSrcOptimal,
                    .accessMask = A::eTransferRead,
                    .stageMask  = S::eAllTransfer,
                };
            }
            case TransferDst: {
                return {
                    .layout     = hasUnifiedLayouts ? L::eGeneral : L::eTransferDstOptimal,
                    .accessMask = A::eTransferWrite,
                    .stageMask  = S::eAllTransfer,
                };
            }
            case PresentSrc: {
                return {
                    .layout     = L::ePresentSrcKHR,
                    .accessMask = A::eNone,
                    .stageMask  = S::eBottomOfPipe
                };
            }
        }

        nbl_ASSERT(false, "Unknown ImageUsage!");
        std::unreachable();
    }

    /**
     * Get the common image usage flags for a certain usage.
     * @note All Images receive TransferSrc and TransferDst flags by default.
     */
    [[nodiscard]] constexpr vk::ImageUsageFlags getImageUsageFlags(const ImageUsage usage) noexcept
    {
        using enum ImageUsage;
        using enum vk::ImageUsageFlagBits;

        vk::ImageUsageFlags usageFlags = eTransferSrc | eTransferDst;
        switch (usage)
        {
            case ColorAttachment: {
                usageFlags |= eColorAttachment;
                break;
            }
            case DepthAttachment: {
                usageFlags |= eDepthStencilAttachment;
                break;
            }
            case ShaderReadOnly: {
                usageFlags |= eSampled | eStorage;
                break;
            }
            case StorageImage: {
                usageFlags |= eStorage;
                break;
            }
            default: {
                usageFlags = {};
            }
        }

        return usageFlags;
    }

    /**
     * Make an ImageMemoryBarrier with the Src and Dst state fields populated.
     * @note Image and subresource range must be set on the barrier.
     */
    [[nodiscard]] constexpr vk::ImageMemoryBarrier2 makeImageMemoryBarrier(const ImageState& srcState, const ImageState& dstState) noexcept
    {
        return vk::ImageMemoryBarrier2()
            .setOldLayout(srcState.layout)
            .setSrcAccessMask(srcState.accessMask)
            .setSrcStageMask(srcState.stageMask)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setNewLayout(dstState.layout)
            .setDstAccessMask(dstState.accessMask)
            .setDstStageMask(dstState.stageMask)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    }

    // =======================================
    // Image : Immutable Properties
    // =======================================

    struct ImageProperties
    {
        vk::Format              format;
        vk::Extent2D            extent;
        vk::ImageAspectFlags    aspectFlags;
        uint32_t                levelCount  = 1;
        uint32_t                layerCount  = 1;
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
        vk::ImageUsageFlags     usageFlags = {};

        [[nodiscard]] vk::Extent3D getExtent3D() const
        {
            return { extent.width, extent.height, 1 };
        }

        [[nodiscard]] vk::ImageSubresourceLayers getSubresourceLayers() const noexcept
        {
            return vk::ImageSubresourceLayers()
                .setAspectMask(aspectFlags)
                .setBaseArrayLayer(0)
                .setLayerCount(layerCount)
                .setMipLevel(0);
        }

        [[nodiscard]] vk::ImageSubresourceRange getSubresourceRange() const noexcept
        {
            return vk::ImageSubresourceRange()
                .setAspectMask(aspectFlags)
                .setBaseArrayLayer(0)
                .setLayerCount(layerCount)
                .setBaseMipLevel(0)
                .setLevelCount(levelCount);
        }
    };
}
