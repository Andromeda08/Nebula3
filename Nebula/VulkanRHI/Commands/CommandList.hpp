#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct CommandListCreateInfo
    {
        vk::CommandBuffer  commandBuffer;
        bool               singleTimeSubmit = false;
    };

    class CommandList
    {
    public:
        nbl_DISABLE_COPY(CommandList);

        // ❗(Lifetime) mCommandBuffer is allocated by VulkanCommandPool::allocate();
        nbl_CTOR(CommandList);

        // ❗(Lifetime) mCommandBuffer is freed by VulkanCommandPool::free();
        ~CommandList() = default;

        void begin();

        void end();

        [[nodiscard]] vk::CommandBuffer getHandle() const noexcept { return mCommandBuffer; }

    private:
        vk::CommandBuffer   mCommandBuffer;

        bool                mIsRecording = false;
        const bool          mSingleTime  = false;
    };
}