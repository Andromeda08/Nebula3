#include "Instance.hpp"

namespace sunflower::rhi::detail
{
    Instance::Instance(const InstanceCreateInfo& createInfo)
    {
        assert(createInfo.pWindow);

        const auto applicationName = createInfo.applicationName.value_or("SunflowerApp");
        const auto engineName      = createInfo.engineName.value_or("Sunflower");

        const auto applicationInfo = vk::ApplicationInfo()
            .setApiVersion(vk::ApiVersion14)
            .setPApplicationName(applicationName.c_str())
            .setPEngineName(engineName.c_str());

        mExtensions = {
            vk::EXTDebugUtilsExtensionName,
            vk::KHRSurfaceExtensionName,
            vk::KHRGetSurfaceCapabilities2ExtensionName,
            vk::KHRSurfaceMaintenance1ExtensionName,
        };
        mExtensions.append_range(createInfo.pWindow->getVulkanInstanceExtensions());
        if constexpr (conf::gIsMoltenVk)
        {
            mExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
        }

        const bool layerSupport     = evaluateSupport(vk::enumerateInstanceLayerProperties().value, mLayers);
        const bool extensionSupport = evaluateSupport(vk::enumerateInstanceExtensionProperties().value, mExtensions);
        assert(layerSupport && extensionSupport);

        constexpr vk::InstanceCreateFlags createFlags = conf::gIsMoltenVk ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR : vk::InstanceCreateFlags{};;
        const auto instanceCreateInfo = vk::InstanceCreateInfo()
            .setFlags(createFlags)
            .setEnabledExtensionCount(mExtensions.size())
            .setPpEnabledExtensionNames(mExtensions.data())
            .setEnabledLayerCount(mLayers.size())
            .setPpEnabledLayerNames(mLayers.data())
            .setPApplicationInfo(&applicationInfo);

        const auto [result, instance] = vk::createInstance(instanceCreateInfo);
        if (result != vk::Result::eSuccess)
        {
            ::sunflower::exit("Failed to create Vulkan Instance: {}", vk::to_string(result));
        }
        mInstance = instance;
    }

    Instance::~Instance()
    {
        if (mInstance)
        {
            mInstance.destroy();
        }
    }

    const vk::Instance& Instance::getHandle() const noexcept
    {
        return mInstance;
    }
}
