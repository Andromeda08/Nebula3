#pragma once

#include <vulkan/vulkan.hpp>

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

    private:
        SPtr<Device>                 mDevice;
        DeviceQueue                  mQueue;
        std::vector<vk::CommandPool> mCommandPools;
    };
}