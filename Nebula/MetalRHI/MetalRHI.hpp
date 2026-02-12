#pragma once

#include <metal/metal.hpp>
#include "RHI/RHI.hpp"

namespace RHI
{
    class IWindow;
    class MetalSwapchain;

    class MetalRHI : public DynamicRHI
    {
        struct FrameSync
        {
            uint64_t                currentFrame = 0;
            uint64_t                frameNumber  = 0;
            NSPtr<MTL::SharedEvent> sharedEvent;

            void advanceCurrentFrame() noexcept
            {
                currentFrame = (currentFrame + 1) % gFramesInFlight;
            }
        };
    public:
        nbl_DISABLE_COPY(MetalRHI);

        explicit MetalRHI(const RHICreateInfo& createInfo);

        [[nodiscard]] static SPtr<MetalRHI> create(const RHICreateInfo& createInfo) noexcept
        {
            return makeShared<MetalRHI>(createInfo);
        }

        // DynamicRHI Implementation
        // ================================

        [[nodiscard]] FrameData beginFrame() noexcept override;

        void endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const override;

        [[nodiscard]] SPtr<ITexture> createTexture(const TextureCreateInfo& createInfo) noexcept override;

    private:
        SPtr<IWindow>           mWindow;
        NSPtr<MTL::Device>      mDevice;
        SPtr<MetalSwapchain>    mSwapchain;
        FrameSync               mFrameSync;
    };
}
