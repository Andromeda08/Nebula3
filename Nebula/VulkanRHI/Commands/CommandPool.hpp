#pragma once

#include <vulkan/vulkan.hpp>

#include "CommandList.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct CommandPoolCreateInfo
    {
        DeviceQueue  queue;
        SPtr<Device> device;
    };

    class CommandPool
    {
    public:
        nbl_DISABLE_COPY(CommandPool);
        nbl_CTOR_SHARED(CommandPool);

        ~CommandPool() = default;

        CommandList* allocate();
        void         free(const CommandList* pCommandList);
        void         reset() const;

        [[nodiscard]] vk::CommandPool getHandle() const noexcept { return mCommandPool; }

    private:
        vk::CommandPool                mCommandPool;
        std::vector<UPtr<CommandList>> mCommandLists;
        SPtr<Device>                   mDevice;
    };
}
