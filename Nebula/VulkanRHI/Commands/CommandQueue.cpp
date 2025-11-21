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
        // TODO: Implement
    }
}
