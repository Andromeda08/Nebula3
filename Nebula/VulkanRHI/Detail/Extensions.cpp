#include "Extensions.hpp"

#include <string_view>
#include <spdlog/fmt/bundled/color.h>

namespace RHI
{
    struct VulkanAnyStruct
    {
        vk::StructureType sType;
        const void*       pNext;
    };

    namespace detail
    {
        [[nodiscard]] static auto fmtBool(const bool value,
            const std::string_view txtTrue = "Yes", const std::string_view txtFalse = "No") noexcept
        {
            return fmt::styled(value ? txtTrue : txtFalse,
                fg(value ? fmt::color::lawn_green : fmt::color::pale_violet_red)).value;
        }

        [[nodiscard]] constexpr auto getOptionColor(const FeatureOption e) noexcept
        {
            using enum FeatureOption;
            switch (e)
            {
                case Disabled:  return fmt::color::gray;
                case Optional:  return fmt::color::light_gray;
                case Required:  return fmt::color::cadet_blue;
                default:        return fmt::color::white;
            }
        }
    }

    Extension::Extension(const char* extensionName, const FeatureOption option)
    : mOption(option)
    , mExtensionName(extensionName)
    {
        mIsCoreFeatureStruct = std::string(mExtensionName).contains("VulkanCore");
        if (mIsCoreFeatureStruct)
        {
            mSupported = true;
        }
    }

    Extension::Extension(const char* extensionName, const FeatureOption option, const std::function<void()>& featureInitFn)
    : mOption(option)
    , mExtensionName(extensionName)
    , mFeatureInitFn(featureInitFn)
    {
        mIsCoreFeatureStruct = std::string(mExtensionName).contains("VulkanCore");
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
        return mSupported && mOption != FeatureOption::Disabled;
    }

    const char* Extension::getName() const noexcept
    {
        return mExtensionName;
    }

    FeatureOption Extension::getRequestType() const noexcept
    {
        return mOption;
    }

    std::string Extension::toString(const size_t width) const noexcept
    {
        return fmt::format("{:<{}} [Supported={:<3} | {} | {:>8}]",
            mExtensionName, width == 0 ? std::strlen(mExtensionName) : width,
            detail::fmtBool(mSupported),
            styled(RHI::toString(mOption), fg(detail::getOptionColor(mOption))),
            detail::fmtBool(mSupported && mOption != FeatureOption::Disabled, "Active", "Inactive"));
    }
}
