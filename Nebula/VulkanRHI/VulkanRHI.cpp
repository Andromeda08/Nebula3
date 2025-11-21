#include "VulkanRHI.hpp"

#include <print>

#include "Core/ToString.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace RHI
{
    Instance::Instance(const VulkanRHICreateInfo& rhiCreateInfo)
    {
        const auto& config = Configuration::getConfig();
        const auto  applicationInfo = vk::ApplicationInfo()
            .setApiVersion(VK_API_VERSION_1_4)
            .setPApplicationName(config.app.appName.c_str())
            .setPEngineName("Nebula3");

        if (config.rhi.debugFeatures)
        {
            mLayers.push_back(gVulkanValidationLayerName);
            mExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        mExtensions.append_range(Platform::getInstanceExtensions());
        mExtensions.append_range(rhiCreateInfo.pWindow->getVulkanInstanceExtensions());

        const auto layerSupport = evaluateSupport(vk::enumerateInstanceLayerProperties(), mLayers);
        assert(layerSupport);

        const auto extensionSupport = evaluateSupport(vk::enumerateInstanceExtensionProperties(), mExtensions);
        assert(extensionSupport);

        const auto instanceCreateInfo = vk::InstanceCreateInfo()
            .setFlags(Platform::getInstanceFlags())
            .setEnabledExtensionCount(static_cast<uint32_t>(mExtensions.size()))
            .setPpEnabledExtensionNames(mExtensions.data())
            .setEnabledLayerCount(static_cast<uint32_t>(mLayers.size()))
            .setPpEnabledLayerNames(mLayers.data())
            .setPApplicationInfo(&applicationInfo);

        mInstance = vk::createInstance(instanceCreateInfo);
    }

    SPtr<Instance> Instance::create(const VulkanRHICreateInfo& rhiCreateInfo) noexcept
    {
        return std::make_shared<Instance>(rhiCreateInfo);
    }

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

        mInstance = Instance::create(createInfo);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance->getHandle());

        if (config.rhi.debugFeatures)
        {
            mDebugContext = DebugContext::create({ mInstance });
        }

        mDevice = Device::create({ mInstance->getHandle() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice->getHandle());

        mFrameSync = std::make_unique<FrameSync>(mDevice.get());

        std::println("[RHI] Created VulkanRHI\n\t- Device: {}\n\t- Feature Level: {}\n\t- Debug Features: {}",
            mDevice->getDeviceName(), toString(mFeatureLevel), toString(config.rhi.debugFeatures));
    }

}
