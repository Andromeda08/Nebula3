#pragma once

#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"

namespace RHI
{
    class Device;

    struct FrameData
    {
        vk::Fence       waitFence;
        vk::Semaphore   imageReadySemaphore;
        vk::Semaphore   renderingFinishedSemaphore;
        const uint64_t  currentFrame;
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

        explicit FrameSync(const SPtr<Device>& pDevice);

        ~FrameSync();

        void advanceCurrentFrame() noexcept;

    private:
        SPtr<Device> mDevice;
    };
}
