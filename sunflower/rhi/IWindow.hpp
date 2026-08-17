#pragma once

#include <vector>
#include <rhi/Common.hpp>

namespace vk
{
    class Instance;
    class SurfaceKHR;
}

namespace sunflower::rhi
{
    struct IWindow
    {
        sunflower_INTERFACE(IWindow);

        [[nodiscard]] virtual Size getFramebufferSize() = 0;

        virtual void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) = 0;

        [[nodiscard]] virtual std::vector<const char*> getVulkanInstanceExtensions() = 0;
    };
}