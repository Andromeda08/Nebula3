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

    vk::SurfaceCapabilitiesKHR Surface::getCapabilities(const vk::PhysicalDevice& physicalDevice) const
    {
        const auto surfaceInfo = vk::PhysicalDeviceSurfaceInfo2KHR().setSurface(mSurface);

        const auto [result, surfaceCaps] = physicalDevice.getSurfaceCapabilities2KHR(surfaceInfo);
        if (result != vk::Result::eSuccess)
        {
            ::sunflower::exit("Failed to query surface capabilities: {}", vk::to_string((result)));
        }

        return surfaceCaps.surfaceCapabilities;
    }

    vk::SurfaceFormatKHR Surface::getFormat(const vk::PhysicalDevice& physicalDevice, const Format prefFormat, const vk::ColorSpaceKHR prefColorSpace)
    {
        if (mFormats.empty())
        {
            const auto surfaceInfo = vk::PhysicalDeviceSurfaceInfo2KHR().setSurface(mSurface);
            const auto [result, formats] = physicalDevice.getSurfaceFormats2KHR(surfaceInfo);
            if (result != vk::Result::eSuccess)
            {
                ::sunflower::exit("Failed to query surface formats: {}", vk::to_string((result)));
            }
            if (formats.empty())
            {
                ::sunflower::exit("The surface reported no supported formats.");
            }
            mFormats = formats;
        }

        for (const auto& fmt : mFormats)
        {
            const auto& [format, colorSpace] = fmt.surfaceFormat;
            if (format == toVulkan(prefFormat) && colorSpace == prefColorSpace)
            {
                return fmt.surfaceFormat;
            }
        }

        return mFormats[0].surfaceFormat;
    }

    vk::PresentModeKHR Surface::getPresentMode(const vk::PhysicalDevice& physicalDevice, const vk::PresentModeKHR prefPresentMode)
    {
        if (mPresentModes.empty())
        {
            const auto [result, presentModes] = physicalDevice.getSurfacePresentModesKHR(mSurface);
            if (result != vk::Result::eSuccess)
            {
                ::sunflower::exit("Failed to query surface present modes: {}", vk::to_string((result)));
            }
            if (presentModes.empty())
            {
                ::sunflower::exit("The surface reported no supported present modes.");
            }
            mPresentModes = presentModes;
        }

        return std::ranges::find(mPresentModes, prefPresentMode) != std::end(mPresentModes)
            ? prefPresentMode
            : mPresentModes[0];
    }

    const vk::SurfaceKHR& Surface::getHandle() const noexcept
    {
        return mSurface;
    }
}
