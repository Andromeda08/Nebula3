#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "Image.hpp"
#include "VulkanCore.hpp"
#include "Commands/CommandList.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
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

    constexpr ImageState getImageState(const ImageUsage usage)
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
            }
        }
    }

    struct ImageBarrier
    {
        ImageUsage  dstUsage;
        SPtr<Image> image;
    };

    struct BufferBarrier
    {
        SPtr<Buffer> buffer;
    };

    class Barrier
    {
    public:
        Barrier& addImageBarrier(const ImageBarrier& imageBarrier);

        Barrier& addBufferBarrier(const BufferBarrier& bufferBarrier);

        void insert(const CommandList* pCommandList);

        void insert(const vk::CommandBuffer& commandBuffer);

    private:
        vk::DependencyInfo                      mDependencyInfo;

        std::vector<SPtr<Buffer>>               mBuffers;
        std::vector<vk::BufferMemoryBarrier2>   mBufferBarriers;

        std::vector<SPtr<Image>>                mImages;
        std::vector<vk::ImageMemoryBarrier2>    mImageBarriers;
    };
}
