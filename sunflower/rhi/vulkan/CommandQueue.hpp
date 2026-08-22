#pragma once

#include <rhi/Common.hpp>
#include <rhi/vulkan/CommandPool.hpp>
#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi
{
    struct VulkanCommandQueueCreateInfo
    {
        DeviceQueue  queue;
        SPtr<Device> device;
    };

    class VulkanCommandQueue final : public ICommandQueue
    {
    public:
        sunflower_DisableCopy(VulkanCommandQueue);
        sunflower_Create(VulkanCommandQueue, UPtr);

        ~VulkanCommandQueue() override = default;

        Timeline* getTimeline() const override;

        QueueType getQueueType() const noexcept override;

        UPtr<ICommandPool> createCommandPool() override;

        void waitIdle() const override;

        void submit(const SubmitInfo& submitInfo) const override;

        /**
         * Used for present path, Vulkan doesn't support timeline semaphores for it.
         */
        void submitWithBinarySync(const SubmitInfo& submitInfo, const std::vector<vk::Semaphore>& binaryWaits, const std::vector<vk::Semaphore>& binarySignals) const;

        void immediate(const std::function<void(ICommandList*)>& fn) const override;

    private:
        SPtr<Device>            mDevice;
        DeviceQueue             mQueue;
        UPtr<Timeline>          mTimeline;
        UPtr<VulkanCommandPool> mImmediatePool;
    };
}
