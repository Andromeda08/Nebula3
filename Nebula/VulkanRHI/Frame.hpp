#pragma once

#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"
#include "RHI/RHI.hpp"

namespace RHI
{
    class Device;

    struct FrameSync
    {
        // GPU - CPU sync
        PerFrameArray<vk::Fence>      frameInFlight;

        // Swapchain sync
        PerFrameArray<vk::Semaphore>  imageReady;
        PerFrameArray<vk::Semaphore>  renderingFinished;

        uint64_t                      currentFrame;

        explicit FrameSync(const SPtr<Device>& pDevice);

        ~FrameSync();

        void advanceCurrentFrame() noexcept;

    private:
        SPtr<Device> mDevice;
    };
}
