#include "CommandQueue.hpp"

#include "CommandPool.hpp"
#include "VulkanTimelineSync.hpp"

namespace RHI
{
    CommandQueue::CommandQueue(const CommandQueueCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mQueue(createInfo.queue)
    {
        mTimeline = makeUnique<Timeline>(
            makeUnique<VulkanTimelineSync>(mDevice, "Timeline")
        );
        mImmediatePool = CommandPool::create({
            .queue  = mQueue,
            .device = mDevice,
        });
    }

    Timeline* CommandQueue::getTimeline() const
    {
        return mTimeline.get();
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
        const uint64_t value = mTimeline->getNextValue();
        submit(SubmitInfo().addSignal(mTimeline->getSync(), value));
        const bool ok = mTimeline->hostWait(value);
        if (!ok) { exitWithError("waitIdle timed out on queue timeline"); }
    }

    void CommandQueue::submit(const SubmitInfo& submitInfo) const
    {
        submitWithBinarySync(submitInfo, {}, {});
    }

    void CommandQueue::submitWithBinarySync(const SubmitInfo& submitInfo, const std::vector<vk::Semaphore>& binaryWaits, const std::vector<vk::Semaphore>& binarySignals) const
    {
        std::vector<vk::CommandBufferSubmitInfo> commandBufferSubmitInfos;
        for (const auto commandBuffer : submitInfo.commandLists | std::views::transform([](const auto* x){ return rhi_cast<CommandList>(x)->getHandle(); }))
        {
            const auto info = vk::CommandBufferSubmitInfo().setCommandBuffer(commandBuffer);
            commandBufferSubmitInfos.push_back(info);
        }

        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreInfos;
        waitSemaphoreInfos.reserve(submitInfo.waits.size() + binaryWaits.size());

        for (const auto& [pSync, value] : submitInfo.waits)
        {
            waitSemaphoreInfos.push_back(vk::SemaphoreSubmitInfo()
                .setSemaphore(rhi_cast<VulkanTimelineSync>(pSync)->getHandle())
                .setValue(value)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
        }
        for (const auto& binaryWait : binaryWaits)
        {
            waitSemaphoreInfos.push_back(vk::SemaphoreSubmitInfo()
                .setSemaphore(binaryWait)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
        }

        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreInfos;
        signalSemaphoreInfos.reserve(submitInfo.signals.size() + binarySignals.size());

        for (const auto& [pSync, value] : submitInfo.signals)
        {
            signalSemaphoreInfos.push_back(vk::SemaphoreSubmitInfo()
                .setSemaphore(rhi_cast<VulkanTimelineSync>(pSync)->getHandle())
                .setValue(value)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
        }
        for (const auto& binaryWait : binarySignals)
        {
            signalSemaphoreInfos.push_back(vk::SemaphoreSubmitInfo()
                .setSemaphore(binaryWait)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands));
        }

        const auto vkSubmitInfo = vk::SubmitInfo2()
            .setCommandBufferInfos(commandBufferSubmitInfos)
            .setWaitSemaphoreInfos(waitSemaphoreInfos)
            .setSignalSemaphoreInfos(signalSemaphoreInfos);

        mQueue.queue.submit2(vkSubmitInfo);
    }

    void CommandQueue::immediate(const std::function<void(CommandList*)>& fn) const
    {
        auto* commandList = mImmediatePool->allocate();

        commandList->begin();
        fn(commandList);
        commandList->end();

        const uint64_t value = mTimeline->getNextValue();
        submit(SubmitInfo().addCommandList(commandList).addSignal(mTimeline->getSync(), value));

        const bool ok = mTimeline->hostWait(value);
        if (!ok) { exitWithError("immediate() timed out on queue timeline"); }

        mImmediatePool->free(commandList);
    }
}
