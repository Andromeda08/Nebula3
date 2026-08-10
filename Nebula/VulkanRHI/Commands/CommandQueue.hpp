#pragma once

#include <functional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

#include "RHI/Timeline.hpp"

namespace RHI
{
    class CommandPool;
    class CommandList;

    struct CommandQueueCreateInfo
    {
        DeviceQueue  queue;
        SPtr<Device> device;
    };

    class CommandQueue
    {
    public:
        nbl_DISABLE_COPY(CommandQueue);
        nbl_CTOR(CommandQueue);

        ~CommandQueue() = default;

        Timeline* getTimeline() const;

        QueueType         getQueueType() const noexcept;

        SPtr<CommandPool> createCommandPool() const;

        void waitIdle() const;

        void submit(const SubmitInfo& submitInfo) const;

        /**
         * Used for present path, Vulkan doesn't support timeline semaphores for it.
         */
        void submitWithBinarySync(const SubmitInfo& submitInfo, const std::vector<vk::Semaphore>& binaryWaits, const std::vector<vk::Semaphore>& binarySignals) const;

        void immediate(const std::function<void(CommandList*)>& fn) const;

    private:
        SPtr<Device>        mDevice;
        DeviceQueue         mQueue;
        UPtr<Timeline>      mTimeline;
        SPtr<CommandPool>   mImmediatePool;
    };
}