#include "Instance.hpp"

#include "Core/Util.hpp"

namespace RHI
{
    Instance::Instance(const InstanceCreateInfo& createInfo)
    {
        const auto& config = Configuration::getConfig();
        const auto  applicationInfo = vk::ApplicationInfo()
            .setApiVersion(vk::ApiVersion14)
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
        exitOnAssert(layerSupport, "Not all requested Instance layers are supported.");

        const auto extensionSupport = evaluateSupport(vk::enumerateInstanceExtensionProperties(), mExtensions);
        exitOnAssert(extensionSupport, "Not all requested Instance extensions are supported.");

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
