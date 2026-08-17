#include "Surface.hpp"

namespace sunflower::rhi::detail
{
    Surface::Surface(const SurfaceCreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    , mInstance(createInfo.instance)
    {
        assert(mWindow && mInstance);
        mWindow->createVulkanSurface(mInstance->getHandle(), &mSurface);
    }

    Surface::~Surface()
    {
        if (mSurface)
        {
            mInstance->getHandle().destroy(mSurface);
        }
    }

    const vk::SurfaceKHR& Surface::getHandle() const noexcept
    {
        return mSurface;
    }
}
