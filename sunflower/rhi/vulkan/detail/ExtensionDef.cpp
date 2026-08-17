#include "ExtensionDef.hpp"

#include <map>
#include <string_view>
#include <sstream>
#include <spdlog/fmt/bundled/color.h>

// Extension
namespace sunflower::rhi
{
    namespace
    {
        struct VulkanAnyStruct
        {
            vk::StructureType sType;
            const void*       pNext;
        };
    }

    namespace detail
    {
        [[nodiscard]] static auto fmtBool(const bool value, const std::string_view txtTrue = "Yes", const std::string_view txtFalse = "No")
        {
            return fmt::styled(value ? txtTrue : txtFalse, fg(value ? fmt::color::lawn_green : fmt::color::pale_violet_red));
        }

        [[nodiscard]] static constexpr auto getOptionColor(const Req e) noexcept
        {
            using enum Req;
            switch (e)
            {
                case Disabled:  return fmt::color::gray;
                case Optional:  return fmt::color::light_gray;
                case Required:  return fmt::color::cadet_blue;
                default:        return fmt::color::white;
            }
        }
    }

    Extension::Extension(const char* extensionName, const Req option)
    : mOption(option)
    , mExtensionName(extensionName)
    {
        mIsCoreFeatureStruct = std::string(mExtensionName).contains("Vulkan Core");
        if (mIsCoreFeatureStruct)
        {
            mSupported = true;
        }
    }

    Extension::Extension(const char* extensionName, const Req option, const std::function<void()>& featureInitFn)
    : mOption(option)
    , mExtensionName(extensionName)
    , mFeatureInitFn(featureInitFn)
    {
        mIsCoreFeatureStruct = std::string(mExtensionName).contains("Vulkan Core");
        if (mIsCoreFeatureStruct)
        {
            mSupported = true;
        }
    }

    void Extension::preQueryProperties(vk::PhysicalDeviceProperties2& properties2) const noexcept
    {
        if (mPropertiesStructPtr != nullptr && isActive())
        {
            auto* propertiesStruct  = static_cast<VulkanAnyStruct*>(mPropertiesStructPtr);
            propertiesStruct->pNext = properties2.pNext;
            properties2.pNext = mPropertiesStructPtr;
        }
    }

    void Extension::preCreateDevice(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept
    {
        if (mFeatureStructPtr != nullptr && isActive())
        {
            mFeatureInitFn();

            auto* featureStruct  = static_cast<VulkanAnyStruct*>(mFeatureStructPtr);
            featureStruct->pNext = deviceCreateInfo.pNext;
            deviceCreateInfo.setPNext(mFeatureStructPtr);
        }
    }

    void Extension::setSupported() noexcept
    {
        mSupported = true;
    }

    bool Extension::isActive() const noexcept
    {
        return mSupported && mOption != Req::Disabled;
    }

    const char* Extension::getName() const noexcept
    {
        return mExtensionName;
    }

    Req Extension::getRequestType() const noexcept
    {
        return mOption;
    }

    std::string Extension::toString(const size_t width) const noexcept
    {
        return fmt::format("{:<{}} [Supported={:<3} | {} | {:>8}]",
            mExtensionName, width == 0 ? std::strlen(mExtensionName) : width,
            detail::fmtBool(mSupported),
            styled(rhi::toString(mOption), fg(detail::getOptionColor(mOption))),
            detail::fmtBool(mSupported && mOption != Req::Disabled, "Active", "Inactive"));
    }
}

// ExtensionLibrary
namespace sunflower::rhi
{
    ExtensionLibrary::ExtensionLibrary()
    {
        mFeatures = vk::PhysicalDeviceFeatures()
            .setMultiDrawIndirect(true)
            .setFragmentStoresAndAtomics(true)
            .setDrawIndirectFirstInstance(true)
            .setFillModeNonSolid(false)
            .setSamplerAnisotropy(true)
            .setSampleRateShading(true)
            .setShaderInt64(true)
            .setTessellationShader(!conf::gIsApple)
            .setGeometryShader(!conf::gIsApple);

        if constexpr (conf::gIsMoltenVk)
        {
            add("VK_KHR_portability_subset", Req::Required);
        }
    }

    ExtensionLibrary& ExtensionLibrary::add(const char* extensionName, const Req req)
    {
        if (checkIsExtensionRegistered(extensionName))
        {
            return *this;
        }

        mUniqueExtensionNames.insert(extensionName);
        mDeviceExtensions.push_back(makeUnique<Extension>(extensionName, req));

        return *this;
    }

    bool ExtensionLibrary::hasExtension(const char* extensionName) const noexcept
    {
        return mUniqueExtensionNames.contains(extensionName);
    }

    int32_t ExtensionLibrary::evaluateDeviceSupport(const vk::PhysicalDevice& physicalDevice) const
    {
        const std::vector<vk::ExtensionProperties> driverExtensions = physicalDevice.enumerateDeviceExtensionProperties().value;

        int32_t                extensionScore = 0;
        std::map<size_t, bool> support        = {};
        for (auto&& [i, extension] : enumerate(mDeviceExtensions))
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
                    case Req::Required: {
                        extensionScore += scoring::sDeviceScore_HasRequiredExtension;
                        break;
                    }
                    default: {
                        extensionScore += scoring::sDeviceScore_HasOptionalExtension;
                        break;
                    }
                }
            }
            else if (requestType == Req::Required)
            {
                if constexpr (conf::gIsDebug)
                {
                    spdlog::warn("Missing Device extension: {}", extension->getName());
                }
                extensionScore += scoring::sDeviceScore_MissingRequiredExtension;
            }
        }

        return extensionScore;
    }

    void ExtensionLibrary::postPhysicalDeviceSelection(const vk::PhysicalDevice& physicalDevice)
    {
        const std::vector<vk::ExtensionProperties> driverExtensions = physicalDevice.enumerateDeviceExtensionProperties().value;
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

        mPostPhysicalDeviceSelection = true;
    }

    void ExtensionLibrary::preDeviceCreation(vk::DeviceCreateInfo& deviceCreateInfo) const
    {
        for (const auto& extension : mDeviceExtensions)
        {
            extension->preCreateDevice(deviceCreateInfo);
        }
    }

    const vk::PhysicalDeviceProperties& ExtensionLibrary::getProperties() const
    {
        if (!mPostPhysicalDeviceSelection)
        {
            ::sunflower::exit("Only valid after \"postPhysicalDeviceSelection\"!");
        }

        return mProperties.properties;
    }

    const vk::PhysicalDeviceFeatures& ExtensionLibrary::getFeatures() const
    {
        if (!mPostPhysicalDeviceSelection)
        {
            ::sunflower::exit("Only valid after \"postPhysicalDeviceSelection\"!");
        }

        return mFeatures;
    }

    std::vector<std::string_view> ExtensionLibrary::getExtensionNames() const noexcept
    {
        return mUniqueExtensionNames | std::ranges::to<std::vector>();
    }

    const std::vector<Extension*>& ExtensionLibrary::getActiveExtensions() const
    {
        if (!mPostPhysicalDeviceSelection)
        {
            ::sunflower::exit("Only valid after \"postPhysicalDeviceSelection\"!");
        }
        return mActiveExtensions;
    }


    const std::vector<const char*>& ExtensionLibrary::getActiveExtensionNames() const
    {
        if (!mPostPhysicalDeviceSelection)
        {
            ::sunflower::exit("Only valid after \"postPhysicalDeviceSelection\"!");
        }
        return mActiveExtensionNames;
    }

    std::string ExtensionLibrary::toString() const
    {
        if (!mPostPhysicalDeviceSelection)
        {
            ::sunflower::exit("Only valid after \"postPhysicalDeviceSelection\"!");
        }

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

        for (const auto& [i, extension] : enumerate(extensions))
        {
            if (i > 0)
            {
                sstr << std::endl;
            }
            sstr << "\t- " << extension->toString(w);
        }
        return sstr.str();
    }

    bool ExtensionLibrary::checkIsExtensionRegistered(const char* extensionName) const noexcept
    {
        if (mUniqueExtensionNames.contains(extensionName))
        {
            if constexpr (conf::gIsDebug)
            {
                spdlog::warn("The extension '{}' has already been registered.",
                         styled(extensionName, fg(fmt::color::pale_violet_red)));
            }
            return true;
        }
        return false;
    }
}