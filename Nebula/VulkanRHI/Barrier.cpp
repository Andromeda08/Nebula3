#include "Barrier.hpp"

#include <print>

namespace RHI
{
    Barrier& Barrier::addImageBarrier(const ImageBarrier& imageBarrier)
    {
        const auto state    = imageBarrier.image->getState();
        const auto newState = getImageState(imageBarrier.dstUsage);

        const auto barrier  = vk::ImageMemoryBarrier2()
            .setOldLayout(state.layout)
            .setSrcAccessMask(state.accessFlags)
            .setSrcStageMask(state.stageFlags)
            .setNewLayout(newState.layout)
            .setDstAccessMask(newState.accessFlags)
            .setDstStageMask(newState.stageFlags)
            .setSubresourceRange(imageBarrier.image->getProperties().subresourceRange)
            .setImage(imageBarrier.image->getImage());

        mImages.push_back(imageBarrier.image);
        mImageBarriers.push_back(barrier);

        return *this;
    }

    Barrier& Barrier::addBufferBarrier(const BufferBarrier& bufferBarrier)
    {
        std::println("[RHI] Barrier::addBufferBarrier() is not implemented yet.");
        return *this;
    }

    void Barrier::insert(const CommandList* pCommandList)
    {
        insert(pCommandList->getHandle());
    }

    void Barrier::insert(const vk::CommandBuffer& commandBuffer)
    {
        const auto dependencyInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(mImageBarriers.size())
            .setPImageMemoryBarriers(mImageBarriers.data());
        commandBuffer.pipelineBarrier2(dependencyInfo);

        for (const auto& [i, image] : std::views::enumerate(mImages))
        {
            image->updateState({
                mImageBarriers[i].newLayout,
                mImageBarriers[i].dstAccessMask,
                mImageBarriers[i].dstStageMask,
            });
        }
    }
}
