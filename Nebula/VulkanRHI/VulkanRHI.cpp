#include "VulkanRHI.hpp"

#include <print>

#include "Core/ToString.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace RHI
{
    DebugContext::DebugContext(const DebugContextCreateInfo& createInfo)
    : mInstance(createInfo.pInstance)
    {
        using S = vk::DebugUtilsMessageSeverityFlagBitsEXT;
        constexpr auto severity = S::eInfo | S::eWarning | S::eError | S::eVerbose;

        using T = vk::DebugUtilsMessageTypeFlagBitsEXT;
        constexpr auto type = T::eGeneral | T::ePerformance | T::eValidation;

        constexpr auto messengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(severity)
            .setMessageType(type)
            .setPfnUserCallback(debugMessageCallback);

        mMessenger = mInstance->getHandle().createDebugUtilsMessengerEXT(messengerCreateInfo);
    }

    DebugContext::~DebugContext()
    {
        mInstance->getHandle().destroyDebugUtilsMessengerEXT(mMessenger);
    }

    vk::Bool32 DebugContext::debugMessageCallback(
        const vk::DebugUtilsMessageSeverityFlagBitsEXT  severity,
        const vk::DebugUtilsMessageTypeFlagsEXT         type,
        const vk::DebugUtilsMessengerCallbackDataEXT*   p_data,
        void*                                           p_user)
    {
        if (!p_data) return vk::False;
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            const auto msg = std::string(p_data->pMessage);
            std::println("[VulkanValidation] {}", p_data->pMessage);
        }
        return vk::False;
    }

    VulkanRHI::VulkanRHI(const VulkanRHICreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    {
        const auto& config = Configuration::getConfig();
        mFeatureLevel = config.rhi.featureLevel;

        const vk::detail::DynamicLoader dynamicLoader;
        const auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        mInstance = Instance::create({ mWindow.get() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance->getHandle());

        if (config.rhi.debugFeatures)
        {
            mDebugContext = DebugContext::create({ mInstance });
        }

        mDevice = Device::create({ mInstance->getHandle() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice->getHandle());

        mSwapchain = Swapchain::create({
            .window   = mWindow,
            .instance = mInstance,
            .device   = mDevice,
        });

        mFrameSync = std::make_unique<FrameSync>(mDevice.get());

        const auto& swapchainProperties = mSwapchain->getProperties();
        std::println("[RHI] Created VulkanRHI\n\t- Device: {}\n\t- Feature Level: {}\n\t- Debug Features: {}\n\t- Swapchain Details: [images={}, format={}, colorSpace={}, presentMode={}]",
            mDevice->getDeviceName(), toString(mFeatureLevel), toString(config.rhi.debugFeatures),
            mSwapchain->getImageCount(), vk::to_string(swapchainProperties.format), vk::to_string(swapchainProperties.colorSpace), vk::to_string(swapchainProperties.presentMode));
    }

}
