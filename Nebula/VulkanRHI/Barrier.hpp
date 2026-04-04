#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "Image.hpp"
#include "VulkanCore.hpp"
#include "Commands/CommandList.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
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
        Barrier& addBarrier(const vk::ImageMemoryBarrier2& imageBarrier);

        Barrier& addBarrier(const vk::BufferMemoryBarrier2& bufferBarrier);

        Barrier& addImageBarrier(const ImageBarrier& imageBarrier);

        Barrier& addBufferBarrier(const BufferBarrier& bufferBarrier);

        void insert(const CommandList* pCommandList) const;

        void insert(const vk::CommandBuffer& commandBuffer) const;

    private:
        vk::DependencyInfo                      mDependencyInfo;

        std::vector<SPtr<Buffer>>               mBuffers;
        std::vector<vk::BufferMemoryBarrier2>   mBufferBarriers;

        std::vector<SPtr<Image>>                mImages;
        std::vector<vk::ImageMemoryBarrier2>    mImageBarriers;
    };
}
