#pragma once

#include <set>
#include <vector>

#include "Extensions.hpp"
#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    // Extension Management & Utils
    // =============================
    class DeviceExtensions
    {
    public:
        nbl_DISABLE_COPY(DeviceExtensions);
        
        DeviceExtensions();

        // Add & Check Extensions
        // =============================

        DeviceExtensions& addExtension(const char* extensionName, const FeatureOption option) noexcept;

        template <class T>
        requires std::is_base_of_v<Extension, T>
        DeviceExtensions& addExtension(const FeatureOption option) noexcept
        {
            if (checkIsExtensionRegistered(T::sName))
            {
                return *this;
            }

            mUniqueExtensionNames.insert(T::sName);
            mDeviceExtensions.push_back(makeUnique<T>(option));

            return *this;
        }

        DeviceExtensions& addPlatformRequiredExtensions() noexcept;

        [[nodiscard]] bool hasExtension(const char* extensionName) const noexcept;

        template <class T>
        requires std::is_base_of_v<Extension, T>
        [[nodiscard]] bool hasExtension() const noexcept
        {
            return mUniqueExtensionNames.contains(T::sName);
        }

        // Device Creation & Support
        // =============================

        [[nodiscard]] int32_t evaluateDeviceSupport(const vk::PhysicalDevice& physicalDevice) const noexcept;

        void postPhysicalDeviceSelection(const vk::PhysicalDevice& physicalDevice) noexcept;

        void preDeviceCreation(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept;

        // Accessors & Utils
        // =============================

        [[nodiscard]] const vk::PhysicalDeviceProperties& getProperties() const noexcept
        {
            return mProperties.properties;
        }

        [[nodiscard]] const vk::PhysicalDeviceFeatures& getFeatures() const noexcept
        {
            return mFeatures;
        }

        template <class T>
        requires std::is_base_of_v<Extension, T>
        [[nodiscard]] std::optional<std::reference_wrapper<const typename T::PropertiesType>> getExtensionProperties() const noexcept
        {
            auto it = std::ranges::find_if(mDeviceExtensions, [&](const auto& ext) -> bool { return std::string_view{T::sName} == ext->getName(); });

            if (it == std::end(mDeviceExtensions) || !(*it)->isActive() || ((*it)->mPropertiesStructPtr == nullptr))
            {
                return std::nullopt;
            }

            return std::cref(*static_cast<const T::PropertiesType*>((*it)->mPropertiesStructPtr));
        }

        [[nodiscard]] const std::vector<Extension*>& getActiveExtensions() const noexcept
        {
            return mActiveExtensions;
        }

        [[nodiscard]] std::vector<const char*> getExtensionNames() const noexcept;

        [[nodiscard]] const std::vector<const char*>& getActiveExtensionNames() const noexcept;

        [[nodiscard]] std::string toString() const noexcept;

    private:
        [[nodiscard]] bool checkIsExtensionRegistered(const char* extensionName) const noexcept;

        vk::PhysicalDeviceFeatures      mFeatures;

        std::vector<UPtr<Extension>>    mDeviceExtensions;
        std::set<const char*>           mUniqueExtensionNames;

        // ! Valid after "postPhysicalDeviceSelection()" has been called
        std::vector<const char*>        mActiveExtensionNames;
        std::vector<Extension*>         mActiveExtensions;

        // ! Valid after "postPhysicalDeviceSelection()" has been called
        vk::PhysicalDeviceProperties2   mProperties;
    };
}
