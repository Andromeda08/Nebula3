#include "VulkanTimelineSync.hpp"

namespace sunflower::rhi::detail
{
    VulkanTimelineSync::VulkanTimelineSync(const SPtr<Device>& device, const std::string& label): mDevice(device)
    {
        constexpr auto typeInfo = vk::SemaphoreTypeCreateInfo()
            .setSemaphoreType(vk::SemaphoreType::eTimeline)
            .setInitialValue(0);

        const auto [result, semaphore] = mDevice->getHandle().createSemaphore(vk::SemaphoreCreateInfo().setPNext(&typeInfo));
        if (result != vk::Result::eSuccess)
        {
            ::sunflower::exit("Failed to create semaphore: {}", vk::to_string(result));
        }
        mSemaphore = semaphore;

        mDevice->setLabel(mSemaphore, label);
    }

    VulkanTimelineSync::~VulkanTimelineSync()
    {
        mDevice->getHandle().destroySemaphore(mSemaphore);
    }

    uint64_t VulkanTimelineSync::getSignaledValue() const
    {
        return mDevice->getHandle().getSemaphoreCounterValue(mSemaphore).value;
    }

    bool VulkanTimelineSync::hostWait(const uint64_t value, const uint64_t timeoutNs) const
    {
        const auto waitInfo = vk::SemaphoreWaitInfo()
                              .setSemaphores(mSemaphore)
                              .setValues(value);
        return mDevice->getHandle().waitSemaphores(waitInfo, timeoutNs) == vk::Result::eSuccess;
    }

    void VulkanTimelineSync::hostSignal(const uint64_t value) const
    {
        std::ignore = mDevice->getHandle().signalSemaphore(vk::SemaphoreSignalInfo()
            .setSemaphore(mSemaphore)
            .setValue(value));
    }
}
