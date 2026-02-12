#include "MetalSwapchain.hpp"

#include <spdlog/spdlog.h>
#include "RHI/IWindow.hpp"

namespace RHI
{
    MetalSwapchain::MetalSwapchain(const MetalSwapchainCreateInfo& createInfo)
    : ISwapchain()
    , mCurrentDrawable(nullptr)
    , mWindow(createInfo.window)
    , mDevice(createInfo.device)
    {
        mExtent = createInfo.window->getFramebufferSize();

        CA::MetalLayer* pMetalLayer = mWindow->getMetalLayer();
        mMetalLayer                 = NS::TransferPtr(pMetalLayer);

        mMetalLayer->setDevice(mDevice.get());
        mMetalLayer->setPixelFormat(to_mtl(createInfo.preferences.format));
        mMetalLayer->setMaximumDrawableCount(gFramesInFlight);
        mMetalLayer->setDrawableSize(CGSizeMake(mExtent.width, mExtent.height));

        spdlog::debug("[MetalRHI] Acquired MetalLayer");
    }

    uint32_t MetalSwapchain::acquireNext() noexcept
    {
        mCurrentDrawable = mMetalLayer->nextDrawable();
        return mCurrentDrawable->drawableID();
    }

    void MetalSwapchain::presentDrawable() const noexcept
    {
        mCurrentDrawable->present();
    }
}
