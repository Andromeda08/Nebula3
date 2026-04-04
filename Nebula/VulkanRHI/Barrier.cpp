#include "Barrier.hpp"

#include <print>

namespace RHI
{
    Barrier& Barrier::addBarrier(const vk::ImageMemoryBarrier2& imageBarrier)
    {
        mImageBarriers.push_back(imageBarrier);
        return *this;
    }

    Barrier& Barrier::addBarrier(const vk::BufferMemoryBarrier2& bufferBarrier)
    {
        mBufferBarriers.push_back(bufferBarrier);
        return *this;
    }

    Barrier& Barrier::addImageBarrier(const ImageBarrier& imageBarrier)
    {
        const auto barrier = imageBarrier.image->getBarrier(imageBarrier.dstUsage);

        mImages.push_back(imageBarrier.image);
        mImageBarriers.push_back(barrier);

        return *this;
    }

    Barrier& Barrier::addBufferBarrier(const BufferBarrier& bufferBarrier)
    {
        std::println("[RHI] Barrier::addBufferBarrier() is not implemented yet.");
        return *this;
    }

    void Barrier::insert(const CommandList* pCommandList) const
    {
        insert(pCommandList->getHandle());
    }

    void Barrier::insert(const vk::CommandBuffer& commandBuffer) const
    {
        const auto dependencyInfo = vk::DependencyInfo()
            .setImageMemoryBarriers(mImageBarriers)
            .setBufferMemoryBarriers(mBufferBarriers);
        commandBuffer.pipelineBarrier2(dependencyInfo);
    }
}
