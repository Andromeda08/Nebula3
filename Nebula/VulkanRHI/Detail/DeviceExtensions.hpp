#pragma once

#include <set>
#include <vector>

#include "Extensions.hpp"
#include "Core/Macro.hpp"
#include "Core/Ranges.hpp"
#include "Core/Types.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    template <class T>
    concept IsExtClass = std::is_base_of_v<Extension, T>;

    // Extension Management & Utils
    // =============================
    class DeviceExtensions
    {
    public:
        nbl_DISABLE_COPY(DeviceExtensions);
        
        DeviceExtensions();

        // Add & Check Extensions
        // ============================================================
        #pragma region

        /**
         * Add an extension by its name.
         */
        DeviceExtensions& addExtension(const char* extensionName, FeatureOption option) noexcept;

        /**
         * Add an extension by its representative class.
         */
        template <IsExtClass T>
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

        /**
         * Add the extensions required by the current platform. (e.g. portability on Apple)
         */
        DeviceExtensions& addPlatformRequiredExtensions() noexcept;

        /**
         * Does the extension list contain the specified extension.
         * @param extensionName
         */
        [[nodiscard]] bool hasExtension(const char* extensionName) const noexcept;

        /**
         * Does the extension list contain the specified extension.
         * @tparam T Extension class
         */
        template <IsExtClass T>
        [[nodiscard]] bool hasExtension() const noexcept
        {
            return mUniqueExtensionNames.contains(T::sName);
        }

        /**
         * Check if the specified extension is active.
         * @param extensionName
         */
        [[nodiscard]] bool isActive(const char* extensionName) const
        {
            return contains(mActiveExtensionNames, extensionName);
        }

        /**
         * Check if the specified extension is active.
         * @tparam T Extension class
         */
        template <IsExtClass T>
        [[nodiscard]] bool isActive() const
        {
            return contains(mActiveExtensionNames, T::sName);
        }

        #pragma endregion

        // Device Creation & Extension Support
        // ============================================================
        #pragma region

        /**
         * Computes a score for the specified physical device based, for a positive score
         * the given physical device must support the required extensions and feature set.
         * @param physicalDevice
         * @return Final score
         */
        [[nodiscard]] int32_t evaluateDeviceSupport(const vk::PhysicalDevice& physicalDevice) const noexcept;

        /**
         * Set supported extensions as supported, find and store active extensions and query properties.
         */
        void postPhysicalDeviceSelection(const vk::PhysicalDevice& physicalDevice) noexcept;

        /**
         * Currently only used to chain feature structs to DeviceCreateInfo.
         */
        void preDeviceCreation(vk::DeviceCreateInfo& deviceCreateInfo) const noexcept;

        #pragma endregion

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

        template <IsExtClass T>
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
