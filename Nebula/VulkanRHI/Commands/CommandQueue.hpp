#pragma once

#include <functional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    class CommandPool;
    class CommandList;

    struct SubmitInfo
    {
        std::vector<CommandList*>   commandLists;
        std::vector<vk::Semaphore>  waitSemaphores;
        std::vector<vk::Semaphore>  signalSemaphores;
        vk::PipelineStageFlags2     waitStages {vk::PipelineStageFlagBits2::eAllCommands};
        vk::Fence                   fence {nullptr};
    };

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

        QueueType         getQueueType() const noexcept;
        SPtr<CommandPool> createCommandPool() const;
        void              waitIdle() const;
        void              submit(const SubmitInfo& submitInfo);

        void immediate(const std::function<void(const CommandList*)>& fn) const;

    private:
        SPtr<Device>        mDevice;
        DeviceQueue         mQueue;
        SPtr<CommandPool>   mImmediatePool;
    };
}