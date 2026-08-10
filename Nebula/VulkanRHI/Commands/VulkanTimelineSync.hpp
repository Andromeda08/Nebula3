#pragma once

#include <vulkan/vulkan.hpp>

#include "RHI/Timeline.hpp"
#include "VulkanRHI/Device.hpp"

namespace RHI
{
    class VulkanTimelineSync final : public ITimelineSync
    {
    public:
        explicit VulkanTimelineSync(const SPtr<Device>& device, const std::string& label)
        : mDevice(device)
        {
            constexpr auto typeInfo = vk::SemaphoreTypeCreateInfo()
                .setSemaphoreType(vk::SemaphoreType::eTimeline)
                .setInitialValue(0);

            mSemaphore = mDevice->getHandle().createSemaphore(vk::SemaphoreCreateInfo().setPNext(&typeInfo));
            device->nameObject<vk::Semaphore>({
                .debugName = label,
                .handle = mSemaphore,
            });
        }

        ~VulkanTimelineSync() override
        {
            mDevice->getHandle().destroySemaphore(mSemaphore);
        }

        [[nodiscard]] uint64_t getSignaledValue() const override
        {
            return mDevice->getHandle().getSemaphoreCounterValue(mSemaphore);
        }

        [[nodiscard]] bool hostWait(const uint64_t value, const uint64_t timeoutNs) const override
        {
            const auto waitInfo = vk::SemaphoreWaitInfo()
                .setSemaphores(mSemaphore)
                .setValues(value);
            return mDevice->getHandle().waitSemaphores(waitInfo, timeoutNs) == vk::Result::eSuccess;
        }

        void hostSignal(const uint64_t value) const override
        {
            mDevice->getHandle().signalSemaphore(vk::SemaphoreSignalInfo()
                .setSemaphore(mSemaphore)
                .setValue(value));
        }

        [[nodiscard]] vk::Semaphore getHandle() const noexcept { return mSemaphore; }

    private:
        SPtr<Device>  mDevice;
        vk::Semaphore mSemaphore;
    };
}