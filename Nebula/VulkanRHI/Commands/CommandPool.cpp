#include "CommandPool.hpp"

namespace RHI
{
    CommandPool::CommandPool(const CommandPoolCreateInfo& createInfo)
    : mDevice(createInfo.device)
    {
        const auto poolCreateInfo = vk::CommandPoolCreateInfo()
           .setQueueFamilyIndex(createInfo.queue.familyIndex)
           .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        mCommandPool = mDevice->getHandle().createCommandPool(poolCreateInfo);
    }

    CommandList* CommandPool::allocate()
    {
        const auto allocInfo = vk::CommandBufferAllocateInfo()
            .setCommandBufferCount(1)
            .setCommandPool(mCommandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        const auto allocatedBuffers = mDevice->getHandle().allocateCommandBuffers(allocInfo);

        auto commandList = CommandList::create({
            .commandBuffer    = allocatedBuffers[0],
            .singleTimeSubmit = false,
        });
        mCommandLists.push_back(std::move(commandList));

        auto* pList = mCommandLists[mCommandLists.size() - 1].get();
        return pList;
    }

    void CommandPool::free(const CommandList* pCommandList)
    {
        const auto commandBuffer = pCommandList->getHandle();
        mDevice->getHandle().freeCommandBuffers(mCommandPool, 1, &commandBuffer);
        // TODO: mCommandLists?
    }

    void CommandPool::reset() const
    {
        mDevice->getHandle().resetCommandPool(mCommandPool);
    }
}
