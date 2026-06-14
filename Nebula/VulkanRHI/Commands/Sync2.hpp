#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/Util.hpp"
#include "VulkanRHI/Common.hpp"
#include "VulkanRHI/Image.hpp"

namespace RHI
{
    constexpr bool gBarrierValidation = true;

    class Buffer;
    class Image;

    struct SyncState
    {
        vk::AccessFlags2        access = vk::AccessFlagBits2::eNone;
        vk::PipelineStageFlags2 stage  = vk::PipelineStageFlagBits2::eNone;
        vk::ImageLayout         layout = vk::ImageLayout::eUndefined;
    };

    // Image Synchronization
    // =========================================================
    #pragma region

    // Technically this should come from somewhere else, tho only MoltenVK doesn't support it for me :)
    constexpr bool gUnifiedLayouts = !Platform::isApple;

    // Simplifies the image layout to "General" if possible when unified image layouts is available.
    [[nodiscard]] constexpr vk::ImageLayout getLayout(const vk::ImageLayout layout)
    {
        using enum vk::ImageLayout;
        if constexpr (gUnifiedLayouts)
        {
            // Some layouts remain used even with the extension enabled.
            // https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_unified_image_layouts.html
            return layout == eUndefined     || layout == ePreinitialized
                || layout == ePresentSrcKHR || layout == eSharedPresentKHR
                || layout == eAttachmentFeedbackLoopOptimalEXT
                ? layout
                : eGeneral;
        }
        return layout;
    }

    enum class ImageUsage2
    {
        Undefined,
        ColorAttachment,
        DepthAttachment,
        Clear,
        GeneralRead,
        GeneralWrite,
        ShaderReadOnly,
        StorageRead,
        StorageWrite,
        TransferSrc,
        TransferDst,
        PresentSrc,
    };

    // ImageUsage -> String
    [[nodiscard]] constexpr std::string toString(const ImageUsage2 usage)
    {
        using enum ImageUsage2;
        switch (usage)
        {
            case Undefined:         return "Undefined";
            case ColorAttachment:   return "ColorAttachment";
            case DepthAttachment:   return "DepthAttachment";
            case Clear:             return "Clear";
            case GeneralRead:       return "General (Read)";
            case GeneralWrite:      return "General (Write)";
            case ShaderReadOnly:    return "ShaderReadOnly";
            case StorageRead:       return "Storage (Read)";
            case StorageWrite:      return "Storage (Write)";
            case TransferSrc:       return "TransferSrc";
            case TransferDst:       return "TransferDst";
            case PresentSrc:        return "PresentSrc";
            default:                return "Unknown";
        }
    }

    // Does the specified ImageUsage relate to a Write.
    [[nodiscard]] constexpr bool isWrite(const ImageUsage2 usage)
    {
        using enum ImageUsage2;
        return usage == Clear
            || usage == ColorAttachment
            || usage == DepthAttachment
            || usage == GeneralWrite
            || usage == StorageWrite
            || usage == TransferDst;
    }

    // Get the sync state for an Image Usage.
    [[nodiscard]] constexpr SyncState getSyncState(const ImageUsage2 usage)
    {
        using L = vk::ImageLayout;
        using A = vk::AccessFlagBits2;
        using S = vk::PipelineStageFlagBits2;

        using enum ImageUsage2;
        switch (usage)
        {
            // No masks, undefined layout
            case Undefined:
            {
                return {
                    .access = A::eNone,
                    .stage  = S::eNone,
                    .layout = getLayout(L::eUndefined),
                };
            }
            // Read & Write
            case ColorAttachment:
            {
                return {
                    .access = A::eColorAttachmentRead | A::eColorAttachmentWrite,
                    .stage  = S::eColorAttachmentOutput,
                    .layout = getLayout(L::eColorAttachmentOptimal)
                };
            }
            // Read & Write
            case DepthAttachment:
            {
                return {
                    .access = A::eDepthStencilAttachmentRead | A::eDepthStencilAttachmentWrite,
                    .stage  = S::eEarlyFragmentTests | S::eLateFragmentTests,
                    .layout = getLayout(L::eDepthStencilAttachmentOptimal),
                };
            }
            // Synchronized as TransferDst
            case Clear:
            {
                return {
                    .access = A::eTransferWrite,
                    .stage  = S::eClear,
                    .layout = getLayout(L::eTransferDstOptimal),
                };
            }
            // MemoryRead & All Commands, refine stage if possible.
            case GeneralRead:
            {
                return {
                    .access = A::eMemoryRead,
                    .stage  = S::eAllCommands,
                    .layout = getLayout(L::eGeneral),
                };
            }
            // MemoryWrite & All Commands, refine stage if possible.
            case GeneralWrite:
            {
                return {
                    .access = A::eMemoryWrite,
                    .stage  = S::eAllCommands,
                    .layout = getLayout(L::eGeneral),
                };
            }
            // Sampled Read & All Commands, refine stage if possible.
            case ShaderReadOnly:
            {
                return {
                    .access = A::eShaderRead | A::eShaderSampledRead,
                    .stage  = S::eAllCommands,
                    .layout = getLayout(L::eShaderReadOnlyOptimal),
                };
            }
            // Storage Read & All Commands, refine stage if possible.
            case StorageRead:
            {
                return {
                    .access = A::eShaderRead | A::eShaderStorageRead,
                    .stage  = S::eAllCommands,
                    .layout = getLayout(L::eGeneral),
                };
            }
            // Storage Write & All Commands, refine stage if possible.
            case StorageWrite:
            {
                return {
                    .access = A::eShaderWrite | A::eShaderStorageWrite,
                    .stage  = S::eAllCommands,
                    .layout = getLayout(L::eGeneral),
                };
            }
            case TransferSrc:
            {
                return {
                    .access = A::eTransferRead,
                    .stage  = S::eTransfer,
                    .layout = getLayout(L::eTransferSrcOptimal),
                };
            }
            case TransferDst:
            {
                return {
                    .access = A::eTransferWrite,
                    .stage  = S::eTransfer,
                    .layout = getLayout(L::eTransferDstOptimal),
                };
            }
            case PresentSrc:
            {
                return {
                    .access = A::eNone,
                    .stage  = S::eBottomOfPipe,
                    .layout = getLayout(L::ePresentSrcKHR),
                };
            }
            default:
            {
                exitWithError("Unhandled ImageUsage: {}", toString(usage));
            }
        }
    }

    struct ImageSubresourceRange
    {
        uint32_t baseMip   = 0;
        uint32_t lastMip   = VK_REMAINING_MIP_LEVELS;
        uint32_t baseLayer = 0;
        uint32_t lastLayer = VK_REMAINING_ARRAY_LAYERS;
    };

    struct ImageMemoryBarrier
    {
        SyncState             dstState         = {};
        Image*                pImage           = nullptr;
        ImageSubresourceRange subresourceRange = {};
    };

    #pragma endregion

    struct Barrier2
    {
        Barrier2& add(const ImageMemoryBarrier& imageMemoryBarrier)
        {
            if constexpr (gBarrierValidation)
            {
                const auto& [dstState, pImage, subresourceRange] = imageMemoryBarrier;
                if (pImage == nullptr)
                {
                    exitWithError("The Image specified in a barrier was null!");
                }

                // Check mip level
                if (const auto imageMipLevels = pImage->getProperties().levelCount;
                    subresourceRange.baseMip + subresourceRange.lastMip > imageMipLevels)
                {
                    exitWithError("The subresource range specified in a barrier for an image [{}] is incompatible with the mip level range of the image [levels={}]",
                        "TODO", imageMipLevels);
                }

                // Check array level
                if (const auto imageLevels = pImage->getProperties().levelCount;
                    subresourceRange.baseLayer + subresourceRange.lastLayer > imageLevels)
                {
                    exitWithError("The subresource range specified in a barrier for an image [{}] is incompatible with the layers range of the image [layers={}]",
                           "TODO", imageLevels);
                }
            }
        }

    private:
        std::vector<ImageMemoryBarrier> mImageBarriers;
    };
}
