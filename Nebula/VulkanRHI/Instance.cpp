#include "Instance.hpp"

namespace RHI
{
    Instance::Instance(const InstanceCreateInfo& createInfo)
    {
        const auto& config = Configuration::getConfig();
        const auto  applicationInfo = vk::ApplicationInfo()
            .setApiVersion(VK_API_VERSION_1_4)
            .setPApplicationName(config.app.appName.c_str())
            .setPEngineName("Nebula3");

        if (config.rhi.debugFeatures)
        {
            // mLayers.push_back(gVulkanValidationLayerName);
            mExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        mExtensions.append_range(Platform::getInstanceExtensions());
        mExtensions.append_range(createInfo.pWindow->getVulkanInstanceExtensions());

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
}
