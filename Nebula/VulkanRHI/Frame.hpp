#pragma once

#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"

namespace RHI
{
    class Device;

    struct FrameData
    {
        const uint64_t  frameValue;
        const uint64_t  currentFrame;
        const uint64_t  lifetimeFrameCounter;
        const uint32_t  acquiredIndex;
    };

    struct PresentSubmitInfo
    {
        const FrameData     frameData;
        class CommandList*  pCommandList;
    };

    struct FrameSync
    {
        // GPU - CPU sync
        PerFrameArray<vk::Fence>      frameInFlight;

        // Swapchain sync
        PerFrameArray<vk::Semaphore>  imageReady;
        PerFrameArray<vk::Semaphore>  renderingFinished;

        uint64_t                      currentFrame;
        uint64_t                      lifetimeFrameCounter;

        explicit FrameSync(const SPtr<Device>& pDevice);

        ~FrameSync();

        void advanceCurrentFrame() noexcept;

    private:
        SPtr<Device> mDevice;
    };
}
