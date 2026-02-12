#include "MetalRHI.hpp"

#include <chrono>
#include <spdlog/spdlog.h>
#include "MetalSwapchain.hpp"
#include "MetalTexture.hpp"

namespace RHI
{
    MetalRHI::MetalRHI(const RHICreateInfo& createInfo): mWindow(createInfo.window)
    {
        const auto start = std::chrono::high_resolution_clock::now();

        mDevice    = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
        mSwapchain = MetalSwapchain::create({
            .preferences = {},
            .window      = mWindow,
            .device      = mDevice,
        });

        const auto end = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> delta = end - start;
        spdlog::debug("[MetalRHI] Ready ({}s)", delta.count());
    }

    FrameData MetalRHI::beginFrame() noexcept
    {
        mFrameSync.frameNumber += 1;
        if (mFrameSync.frameNumber >= gFramesInFlight)
        {
            const auto prev = mFrameSync.frameNumber - gFramesInFlight;
            mFrameSync.sharedEvent->waitUntilSignaledValue(prev, std::numeric_limits<uint64_t>::max());
        }
        mFrameSync.advanceCurrentFrame();

        const auto acquiredDrawableId = mSwapchain->acquireNext();

        return {
            .currentFrame  = mFrameSync.currentFrame,
            .acquiredIndex = acquiredDrawableId,
        };
    }

    void MetalRHI::endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const
    {
        // mCommandQueue->wait(metalDrawable);
        // mCommandQueue->commit(&cmd, 1);
        // mCommandQueue->signalDrawable(metalDrawable);

        mSwapchain->presentDrawable();

        // mCommandQueue->signalEvent(mSharedEvent.get(), mFrameNumber);
    }

    SPtr<ITexture> MetalRHI::createTexture(const TextureCreateInfo& createInfo) noexcept
    {
        return MetalTexture::create(createInfo, mDevice);
    }
}
