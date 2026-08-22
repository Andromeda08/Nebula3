#pragma once

#include <rhi/Common.hpp>
#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>

namespace sunflower::rhi::detail
{
    class VulkanTimelineSync final : public TimelineSync
    {
    public:
        explicit VulkanTimelineSync(const SPtr<Device>& device, const std::string& label);

        ~VulkanTimelineSync() override;

        [[nodiscard]] uint64_t getSignaledValue() const override;

        [[nodiscard]] bool hostWait(uint64_t value, uint64_t timeoutNs) const override;

        void hostSignal(uint64_t value) const override;

        [[nodiscard]] vk::Semaphore getHandle() const noexcept { return mSemaphore; }

    private:
        SPtr<Device>  mDevice;
        vk::Semaphore mSemaphore;
    };
}
