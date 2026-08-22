#pragma once

#include <rhi/Common.hpp>
#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi
{
    class VulkanCommandPool;

    struct VulkanCommandListCreateInfo
    {
        vk::CommandBuffer   commandBuffer;
        bool                singleTimeSubmit = false;
    };

    class VulkanCommandList final : public ICommandList
    {
    public:
        sunflower_DisableCopy(VulkanCommandList);
        sunflower_Create(VulkanCommandList, UPtr);

        ~VulkanCommandList() override = default;

        [[nodiscard]] vk::CommandBuffer getHandle() const noexcept
        {
            return mHandle;
        }

        void begin() override;

        void end() override;

    private:
        vk::CommandBuffer   mHandle;
        bool                mIsRecording = false;
    };
}
