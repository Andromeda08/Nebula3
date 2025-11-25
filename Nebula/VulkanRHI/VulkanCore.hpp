#pragma once

#include <algorithm>
#include <functional>
#include <print>
#include <ranges>
#include <set>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    constexpr auto gVulkanValidationLayerName = "VK_LAYER_KHRONOS_validation";
    constexpr auto gVulkanPortabilitySubsetExtensionName = "VK_KHR_portability_subset";

    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
    };

    using QueueFamily = uint32_t;
    using QueueIndex  = uint32_t;

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

    namespace Platform
    {
        [[nodiscard]] constexpr vk::InstanceCreateFlags getInstanceFlags() noexcept
        {
            #ifdef __APPLE__
            return vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
            #endif
            return {};
        }

        [[nodiscard]] constexpr std::vector<const char*> getInstanceExtensions() noexcept
        {
            #ifdef __APPLE__
            return { VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
            #endif
            return {};
        }
    }

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
            std::println("[RHI] Error: Missing features!");
        }

        return allSupported;
    }

    inline bool isDepthFormat(const vk::Format format)
    {
        static std::set depthFormats = {
            vk::Format::eD16Unorm, vk::Format::eD32Sfloat,
            vk::Format::eD16UnormS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint,
            vk::Format::eX8D24UnormPack32,
        };
        return depthFormats.contains(format);
    }
}
