#pragma once

#include <rhi/IWindow.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>
#include <rhi/vulkan/detail/Instance.hpp>

namespace sunflower::rhi::detail
{
    struct SurfaceCreateInfo
    {
        IWindow*       pWindow;
        SPtr<Instance> instance;
    };

    class Surface final
    {
    public:
        sunflower_DisableCopy(Surface);
        sunflower_Create(Surface, UPtr);

        ~Surface();

        [[nodiscard]] const vk::SurfaceKHR& getHandle() const noexcept;

    private:
        IWindow*        mWindow;
        SPtr<Instance>  mInstance;
        vk::SurfaceKHR  mSurface;
    };
}
