#pragma once

#include <algorithm>
#include <print>
#include <ranges>
#include <set>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "Common.hpp"

// Platform Specifics
// ============================
namespace RHI::Platform
{
    // Get platform specific Vulkan Instance flags.
    [[nodiscard]] constexpr vk::InstanceCreateFlags getInstanceFlags() noexcept
    {
        if constexpr (isApple)
        {
            return vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        }
        return vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    // Get platform specific Vulkan Instance extensions.
    [[nodiscard]] constexpr std::vector<const char*> getInstanceExtensions() noexcept
    {
        if constexpr (isApple)
        {
            return { VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
        }
        return {};
    }
}

// Utility Functions
// ============================
namespace RHI
{
    constexpr auto gVulkanValidationLayerName = "VK_LAYER_KHRONOS_validation";
    constexpr auto gVulkanPortabilitySubsetExtensionName = "VK_KHR_portability_subset";

    using QueueFamily = std::uint32_t;
    using QueueIndex  = std::uint32_t;

    struct QueueFamilyInfo
    {
        vk::QueueFamilyProperties properties;
        QueueFamily               familyIndex;
    };

    struct DeviceQueue
    {
        vk::Queue       queue;
        QueueFamily     familyIndex;
        QueueIndex      queueIndex;
        QueueType       queueType;
    };

    template <class T>
    bool evaluateSupport(const std::vector<T>& available, const std::vector<const char*>& requested)
    {
        static_assert(std::is_same_v<vk::LayerProperties, T> || std::is_same_v<vk::ExtensionProperties, T>);

        std::vector<const char*> missing;
        const auto allSupported = std::ranges::all_of(requested, [&available, &missing](const char* name) -> bool {
            const auto it = std::ranges::find_if(available, [name](const T& properties) -> bool {
                if constexpr (std::is_same_v<vk::LayerProperties, T>)
                {
                    return std::string_view{ properties.layerName.data() } == name;
                }
                else if constexpr (std::is_same_v<vk::ExtensionProperties, T>)
                {
                    return std::string_view{ properties.extensionName.data() } == name;
                }
            });
            if (it == std::end(available))
            {
                missing.push_back(name);
            }
            return it != std::end(available);
        });

        if (!missing.empty())
        {
            std::println("[RHI] Error: Missing features:");
            for (const auto* feature : missing)
            {
                std::println("\t- {}", feature);
            }
        }

        return allSupported;
    }
}
