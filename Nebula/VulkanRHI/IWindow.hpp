#pragma once

#include <vector>

namespace vk
{
    struct Extent2D;
    class  Instance;
    class  SurfaceKHR;
}

namespace RHI
{
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        virtual vk::Extent2D getFramebufferSize() const = 0;
        virtual std::vector<const char*> getVulkanInstanceExtensions() const = 0;
        virtual void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const = 0;
    };
}
