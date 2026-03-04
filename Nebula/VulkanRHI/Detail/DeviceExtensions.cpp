#include "DeviceExtensions.hpp"

#include <sstream>
#include <spdlog/fmt/bundled/color.h>
#include "Core/Ranges.hpp"

namespace RHI
{
    DeviceExtensions::DeviceExtensions()
    {
        mFeatures = vk::PhysicalDeviceFeatures()
            .setMultiDrawIndirect(true)
            .setDrawIndirectFirstInstance(true)
            .setFillModeNonSolid(true)
            .setSamplerAnisotropy(true)
            .setSampleRateShading(true)
            .setShaderInt64(true)
            .setTessellationShader(!Platform::isApple)
            .setGeometryShader(!Platform::isApple);
    }

    DeviceExtensions& DeviceExtensions::addExtension(const char* extensionName, const FeatureOption option) noexcept
    {
        if (checkIsExtensionRegistered(extensionName))
        {
            return *this;
        }

        mUniqueExtensionNames.insert(extensionName);
        mDeviceExtensions.push_back(makeUnique<Extension>(extensionName, option));

        return *this;
    }

    DeviceExtensions& DeviceExtensions::addPlatformRequiredExtensions() noexcept
    {
        if constexpr (Platform::isApple)
        {
            return addExtension(gVulkanPortabilitySubsetExtensionName, FeatureOption::Required);
        }
        return *this;
    }

    bool DeviceExtensions::hasExtension(const char* extensionName) const noexcept
    {
        return mUniqueExtensionNames.contains(extensionName);
    }

    int32_t DeviceExtensions::evaluateDeviceSupport(const vk::PhysicalDevice& physicalDevice) const noexcept
    {
        const std::vector<vk::ExtensionProperties> driverExtensions =physicalDevice.enumerateDeviceExtensionProperties();

        int32_t                extensionScore = 0;
        std::map<size_t, bool> support        = {};
        for (auto&& [i, extension] : nbl::enumerate(mDeviceExtensions))
        {
            if (extension->mIsCoreFeatureStruct)
            {
                continue;
            }

            const auto requestType = extension->getRequestType();
            support[i] = containsIf(driverExtensions, [&extension](const vk::ExtensionProperties& properties) -> bool {
                return std::string_view{properties.extensionName.data()} == extension->getName();
            });
            // Scoring
            if (support[i])
            {
                switch (requestType)
                {
                    case FeatureOption::Required: {
                        extensionScore += sDeviceScore_HasRequiredExtension;
                        break;
                    }
                    default: {
                        extensionScore += sDeviceScore_HasOptionalExtension;
                        break;
                    }
                }
            }
            else if (requestType == FeatureOption::Required)
            {
                extensionScore += sDeviceScore_MissingRequiredExtension;
            }
        }

        return extensionScore;
    }

    void DeviceExtensions::postPhysicalDeviceSelection(const vk::PhysicalDevice& physicalDevice) noexcept
    {
        const std::vector<vk::ExtensionProperties> driverExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        for (auto& extension : mDeviceExtensions)
        {
            const auto isSupported = containsIf(driverExtensions, [&extension](const vk::ExtensionProperties& properties) -> bool {
                return std::string_view{properties.extensionName.data()} == extension->getName();
            });
            if (isSupported)
            {
                extension->setSupported();
            }
        }

        // Save active extension names
        auto activeExtensions = mDeviceExtensions
            | std::views::filter([](const auto& ext){ return ext->isActive() && !ext->mIsCoreFeatureStruct; });

        mActiveExtensionNames = activeExtensions
            | std::views::transform([](const auto& ext){ return ext->getName(); })
            | std::ranges::to<std::vector>();

        mActiveExtensions = activeExtensions
            | std::views::transform([](const auto& ext){ return ext.get(); })
            | std::ranges::to<std::vector>();

        // Get physical device properties
        mProperties = vk::PhysicalDeviceProperties2();
        for (const auto& extension : mDeviceExtensions)
        {
            extension->preQueryProperties(mProperties);
        }
        physicalDevice.getProperties2(&mProperties);
    }

    void DeviceExtensions::preDeviceCreation(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept
    {
        for (const auto& extension : mDeviceExtensions)
        {
            extension->preCreateDevice(deviceCreateInfo);
        }
    }

    std::vector<const char*> DeviceExtensions::getExtensionNames() const noexcept
    {
        return mUniqueExtensionNames | std::ranges::to<std::vector>();
    }

    const std::vector<const char*>& DeviceExtensions::getActiveExtensionNames() const noexcept
    {
        return mActiveExtensionNames;
    }

    std::string DeviceExtensions::toString() const noexcept
    {
        std::stringstream sstr;

        auto extensions = mDeviceExtensions
            | std::views::filter([](const auto& ext){ return ext->isActive() && !ext->mIsCoreFeatureStruct; })
            | std::views::transform([](const auto& ext){ return ext.get(); })
            | std::ranges::to<std::vector>();

        size_t w = 0;
        for (const auto& ext : extensions)
        {
            w = std::max(std::strlen(ext->getName()), w);
        }

        for (const auto& [i, extension] : nbl::enumerate(extensions))
        {
            if (i > 0)
            {
                sstr << std::endl;
            }
            sstr << "\t- " << extension->toString(w);
        }
        return sstr.str();
    }

    bool DeviceExtensions::checkIsExtensionRegistered(const char* extensionName) const noexcept
    {
        if (mUniqueExtensionNames.contains(extensionName))
        {
            spdlog::warn("The extension '{}' has already been registered.",
                         styled(extensionName, fg(fmt::color::pale_violet_red)));
            return true;
        }
        return false;
    }
}
