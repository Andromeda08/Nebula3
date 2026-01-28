#include "Frame.hpp"

#include "Device.hpp"

namespace RHI
{
    FrameSync::FrameSync(const SPtr<Device>& pDevice)
    : currentFrame(0)
    , mDevice(pDevice)
    {
        constexpr auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
        constexpr auto fenceCreateInfo     = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

        const auto d = mDevice->getHandle();
        for (uint64_t i = 0; i < gFramesInFlight; i++)
        {
            frameInFlight[i]     = d.createFence(fenceCreateInfo);
            imageReady[i]        = d.createSemaphore(semaphoreCreateInfo);
            renderingFinished[i] = d.createSemaphore(semaphoreCreateInfo);
        }
    }

    FrameSync::~FrameSync()
    {
        const auto d = mDevice->getHandle();
        d.waitIdle();
        for (uint64_t i = 0; i < gFramesInFlight; i++)
        {
            d.destroyFence(frameInFlight[i]);
            d.destroySemaphore(imageReady[i]);
            d.destroySemaphore(renderingFinished[i]);
        }
    }

    void FrameSync::advanceCurrentFrame() noexcept
    {
        currentFrame = (currentFrame + 1) % gFramesInFlight;
    }
}
