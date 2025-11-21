#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Core/Types.hpp"

namespace RHI
{
    struct FrameData
    {
        vk::Fence       waitFence;
        vk::Semaphore   imageReadySemaphore;
        vk::Semaphore   renderingFinishedSemaphore;
        const uint64_t  currentFrame;
        const uint64_t  acquiredIndex;
    };

    struct FrameSync
    {
        // GPU - CPU sync
        PerFrameArray<vk::Fence>      frameInFlight;

        // Swapchain sync
        PerFrameArray<vk::Semaphore>  imageReady;
        PerFrameArray<vk::Semaphore>  renderingFinished;

        uint64_t                      currentFrame;

        explicit FrameSync(Device* pDevice)
        : currentFrame(0)
        , mDevice(pDevice)
        {
            constexpr auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
            constexpr auto fenceCreateInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

            const auto d = mDevice->getHandle();
            for (uint64_t i = 0; i < gFramesInFlight; i++)
            {
                frameInFlight[i]     = d.createFence(fenceCreateInfo);
                imageReady[i]        = d.createSemaphore(semaphoreCreateInfo);
                renderingFinished[i] = d.createSemaphore(semaphoreCreateInfo);
            }
        }

        ~FrameSync()
        {
            const auto d = mDevice->getHandle();
            for (uint64_t i = 0; i < gFramesInFlight; i++)
            {
                d.destroyFence(frameInFlight[i]);
                d.destroySemaphore(imageReady[i]);
                d.destroySemaphore(renderingFinished[i]);
            }
        }

    private:
        Device* mDevice;
    };
}
