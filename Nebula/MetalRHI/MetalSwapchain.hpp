#pragma once

#include <metal/metal.hpp>
#include "RHI/RHI.hpp"

namespace RHI
{
    struct MetalSwapchainCreateInfo
    {
        SwapchainPreferences preferences = {};
        SPtr<IWindow>        window      = nullptr;
        NSPtr<MTL::Device>   device      = nullptr;
    };

    class MetalSwapchain : public ISwapchain
    {
    public:
        nbl_DISABLE_COPY(MetalSwapchain);
        nbl_CTOR(MetalSwapchain);

        [[nodiscard]] uint32_t acquireNext() noexcept;

        void presentDrawable() const noexcept;

    private:
        CA::MetalDrawable*      mCurrentDrawable;

        Extent2D                mExtent;
        NSPtr<CA::MetalLayer>   mMetalLayer;

        SPtr<IWindow>           mWindow;
        NSPtr<MTL::Device>      mDevice;
    };
}