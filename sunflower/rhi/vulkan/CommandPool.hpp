#pragma once

#include <rhi/Common.hpp>
#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/CommandList.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi
{
    struct VulkanCommandPoolCreateInfo
    {
        DeviceQueue  queue;
        SPtr<Device> device;
    };

    class VulkanCommandPool final : public ICommandPool
    {
    public:
        sunflower_DisableCopy(VulkanCommandPool);
        sunflower_Create(VulkanCommandPool, UPtr);

        ~VulkanCommandPool() override;

        ICommandList* allocate() override;

        void free(const ICommandList* pCommandList) override;

        void reset() override;

        [[nodiscard]] vk::CommandPool getHandle() const noexcept { return mCommandPool; }

    private:
        SPtr<Device>                            mDevice;
        DeviceQueue                             mDeviceQueue;
        vk::CommandPool                         mCommandPool;
        std::vector<UPtr<VulkanCommandList>>    mCommandLists;
    };
}
