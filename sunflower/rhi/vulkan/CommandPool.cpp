#include "CommandPool.hpp"

namespace sunflower::rhi
{
    VulkanCommandPool::VulkanCommandPool(const VulkanCommandPoolCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mDeviceQueue(createInfo.queue)
    {
        const auto poolCreateInfo = vk::CommandPoolCreateInfo()
           .setQueueFamilyIndex(mDeviceQueue.familyIndex)
           .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        mCommandPool = mDevice->getHandle().createCommandPool(poolCreateInfo).value;
    }

    VulkanCommandPool::~VulkanCommandPool()
    {
        std::ignore = mDeviceQueue.queue.waitIdle();
        
        mCommandLists.clear();
        mDevice->getHandle().destroy(mCommandPool);
    }

    ICommandList* VulkanCommandPool::allocate()
    {
        const auto allocInfo = vk::CommandBufferAllocateInfo()
            .setCommandBufferCount(1)
            .setCommandPool(mCommandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        const auto allocatedBuffers = mDevice->getHandle().allocateCommandBuffers(allocInfo).value;

        auto commandList = VulkanCommandList::create({
            .commandBuffer    = allocatedBuffers[0],
            .singleTimeSubmit = false,
        });
        mCommandLists.push_back(std::move(commandList));

        auto* pList = mCommandLists[mCommandLists.size() - 1].get();
        return pList;
    }

    void VulkanCommandPool::free(const ICommandList* pCommandList)
    {
        const auto commandBuffer = rhi_cast<VulkanCommandList>(pCommandList)->getHandle();
        mDevice->getHandle().freeCommandBuffers(mCommandPool, commandBuffer);

        std::erase_if(mCommandLists, [&](const auto& list) { return list.get() == pCommandList; });
    }

    void VulkanCommandPool::reset()
    {
        std::ignore = mDevice->getHandle().resetCommandPool(mCommandPool);
    }
}
