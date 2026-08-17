#include "VulkanRHI.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace sunflower::rhi
{
    VulkanRHI::VulkanRHI(const VulkanRHICreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    {
        assert(mWindow);

        const vk::detail::DynamicLoader dynamicLoader;
        const auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        mInstance = detail::Instance::create({
            .pWindow         = mWindow,
            .applicationName = createInfo.applicationName,
            .engineName      = createInfo.engineName,
        });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance->getHandle());

        mSurface = detail::Surface::create({
            .pWindow  = mWindow,
            .instance = mInstance,
        });

        mDevice = Device::create({
            .instance = mInstance,
            .pSurface = mSurface.get(),
        });
    }
}
