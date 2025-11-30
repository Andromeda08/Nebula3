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
