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

        [[nodiscard]] vk::SurfaceCapabilitiesKHR getCapabilities(const vk::PhysicalDevice& physicalDevice) const;

        [[nodiscard]] vk::SurfaceFormatKHR getFormat(const vk::PhysicalDevice& physicalDevice, Format prefFormat, vk::ColorSpaceKHR prefColorSpace);

        [[nodiscard]] vk::PresentModeKHR getPresentMode(const vk::PhysicalDevice& physicalDevice, vk::PresentModeKHR prefPresentMode);

        [[nodiscard]] const vk::SurfaceKHR& getHandle() const noexcept;

    private:
        IWindow*        mWindow;
        SPtr<Instance>  mInstance;
        vk::SurfaceKHR  mSurface;

        // Cached after first call to getFormat.
        std::vector<vk::SurfaceFormat2KHR> mFormats;

        // Cached after first call to getPresentMode.
        std::vector<vk::PresentModeKHR> mPresentModes;
    };
}
