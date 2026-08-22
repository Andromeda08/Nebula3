#include "CommandQueue.hpp"

#include <rhi/vulkan/detail/VulkanTimelineSync.hpp>

namespace sunflower::rhi
{
    VulkanCommandQueue::VulkanCommandQueue(const VulkanCommandQueueCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mQueue(createInfo.queue)
    {
        mTimeline = makeUnique<Timeline>(
            makeUnique<detail::VulkanTimelineSync>(mDevice, "Timeline")
        );
        mImmediatePool = VulkanCommandPool::create({
            .queue  = mQueue,
            .device = mDevice,
        });
    }

    Timeline* VulkanCommandQueue::getTimeline() const
    {
        return mTimeline.get();
    }

    QueueType VulkanCommandQueue::getQueueType() const noexcept
    {
        return mQueue.queueType;
    }

    UPtr<ICommandPool> VulkanCommandQueue::createCommandPool()
    {
        return VulkanCommandPool::create({
            .queue  = mQueue,
            .device = mDevice,
        });
    }

    void VulkanCommandQueue::waitIdle() const
    {
        const uint64_t value = mTimeline->getNextValue();
        submit(SubmitInfo().addSignal(mTimeline->getSync(), value));
        if (const bool ok = mTimeline->hostWait(value); !ok)
        {
            ::sunflower::exit("immediate() timed out on queue timeline");
        }
    }

    void VulkanCommandQueue::submit(const SubmitInfo& submitInfo) const
    {
        submitWithBinarySync(submitInfo, {}, {});
    }

    void VulkanCommandQueue::submitWithBinarySync(const SubmitInfo& submitInfo, const std::vector<vk::Semaphore>& binaryWaits, const std::vector<vk::Semaphore>& binarySignals) const
    {
        std::vector<vk::CommandBufferSubmitInfo> commandBufferSubmitInfos;
        for (const auto commandBuffer : submitInfo.commandLists | std::views::transform([](auto* x){ return rhi_cast<VulkanCommandList>(x)->getHandle(); }))
        {
            const auto info = vk::CommandBufferSubmitInfo().setCommandBuffer(commandBuffer);
            commandBufferSubmitInfos.push_back(info);
        }

        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreInfos;
        waitSemaphoreInfos.reserve(submitInfo.waits.size() + binaryWaits.size());

        for (const auto& [pSync, value] : submitInfo.waits)
        {
            waitSemaphoreInfos.push_back(vk::SemaphoreSubmitInfo()
                .setSemaphore(rhi_cast<detail::VulkanTimelineSync>(pSync)->getHandle())
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
                .setSemaphore(rhi_cast<detail::VulkanTimelineSync>(pSync)->getHandle())
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

        std::ignore = mQueue.queue.submit2(vkSubmitInfo);
    }

    void VulkanCommandQueue::immediate(const std::function<void(ICommandList*)>& fn) const
    {
        auto* commandList = mImmediatePool->allocate();

        commandList->begin();
        fn(commandList);
        commandList->end();

        const uint64_t value = mTimeline->getNextValue();
        submit(SubmitInfo().addCommandList(commandList).addSignal(mTimeline->getSync(), value));

        if (const bool ok = mTimeline->hostWait(value); !ok)
        {
            ::sunflower::exit("immediate() timed out on queue timeline");
        }

        mImmediatePool->free(commandList);
    }
}
