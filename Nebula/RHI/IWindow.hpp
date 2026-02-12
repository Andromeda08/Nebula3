#pragma once

#ifdef nbl_VulkanRHI
#include <vector>

namespace vk
{
    class  Instance;
    class  SurfaceKHR;
}
#endif

#ifdef nbl_MetalRHI
namespace CA
{
    class MetalLayer;
}
#endif

namespace RHI
{
    struct Extent2D;

    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        virtual Extent2D getFramebufferSize() const = 0;

        #ifdef nbl_VulkanRHI
        virtual std::vector<const char*> getVulkanInstanceExtensions() const = 0;

        virtual void createVulkanSurface(const vk::Instance& instance, vk::SurfaceKHR* pSurface) const = 0;
        #endif

        #ifdef nbl_MetalRHI
        virtual CA::MetalLayer* getMetalLayer() const = 0;
        #endif
    };
}
