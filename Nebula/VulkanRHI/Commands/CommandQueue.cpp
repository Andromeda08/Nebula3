#include "CommandQueue.hpp"

#include "CommandPool.hpp"

namespace RHI
{
    CommandQueue::CommandQueue(const CommandQueueCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mQueue(createInfo.queue)
    {
    }

    QueueType CommandQueue::getQueueType() const noexcept
    {
        return mQueue.queueType;
    }

    SPtr<CommandPool> CommandQueue::createCommandPool() const
    {
        return CommandPool::create({
            .queue  = mQueue,
            .device = mDevice,
        });
    }

    void CommandQueue::waitIdle() const
    {
        mQueue.queue.waitIdle();
    }

    void CommandQueue::submit(const SubmitInfo& submitInfo)
    {
        std::vector<vk::CommandBufferSubmitInfo> commandBufferSubmitInfos;
        for (const auto commandBuffer : submitInfo.commandLists | std::views::transform([](const auto* x){ return x->getHandle(); }))
        {
            const auto info = vk::CommandBufferSubmitInfo()
                .setCommandBuffer(commandBuffer);
            commandBufferSubmitInfos.push_back(info);
        }

        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreInfos;
        for (const auto waitSemaphore : submitInfo.waitSemaphores)
        {
            const auto waitSemaphoreInfo = vk::SemaphoreSubmitInfo()
                .setSemaphore(waitSemaphore)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
            waitSemaphoreInfos.push_back(waitSemaphoreInfo);
        }

        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreInfos;
        for (const auto signalSemaphore : submitInfo.signalSemaphores)
        {
            const auto signalSemaphoreInfo = vk::SemaphoreSubmitInfo()
                .setSemaphore(signalSemaphore)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        }

        const auto vkSubmitInfo = vk::SubmitInfo2()
            .setCommandBufferInfos(commandBufferSubmitInfos)
            .setCommandBufferInfoCount(commandBufferSubmitInfos.size())
            .setWaitSemaphoreInfos(waitSemaphoreInfos)
            .setWaitSemaphoreInfoCount(waitSemaphoreInfos.size())
            .setSignalSemaphoreInfos(signalSemaphoreInfos)
            .setSignalSemaphoreInfoCount(signalSemaphoreInfos.size());

        mQueue.queue.submit2(vkSubmitInfo, submitInfo.fence);
    }
}
