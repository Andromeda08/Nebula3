#pragma once

#include <ranges>
#include <string_view>
#include <vector>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <spdlog/spdlog.h>

#ifdef SUNFLOWER_DEBUG
#include <spdlog/fmt/bundled/xchar.h>
#endif

#include <rhi/Common.hpp>
#include <rhi/DynamicRHI.hpp>

namespace sunflower::rhi
{
    struct DeviceQueue
    {
        vk::Queue                 queue;
        uint32_t                  familyIndex;
        uint32_t                  queueIndex;
        QueueType                 queueType;
        vk::QueueFamilyProperties properties;
    };

    namespace detail
    {
        template <class T>
        [[nodiscard]] std::string_view nameOf(const T& props)
        {
            static_assert(std::is_same_v<vk::LayerProperties, T> || std::is_same_v<vk::ExtensionProperties, T>);

            if constexpr (std::is_same_v<vk::LayerProperties, T>)
            {
                return { props.layerName.data() };
            }
            else
            {
                return { props.extensionName.data() };
            }
        }
    }

    /**
     * Evaluate support for the list of requested features compared to the one reported by the driver.
     * In debug mode, if not all requested features are supported a message listing their names is logged.
     * @tparam T Layer or ExtensionProperties
     * @param available driver reported feature set
     * @param requested user requested feature set
     * @return true if all requested features are available
     */
    template <class T>
    bool evaluateSupport(const std::vector<T>& available, const std::vector<const char*>& requested)
    {
        static_assert(std::is_same_v<vk::LayerProperties, T> || std::is_same_v<vk::ExtensionProperties, T>);

        std::vector<const char*> missing;
        missing.reserve(requested.size());

        for (const char* name : requested)
        {
            bool found = false;
            for (const T& props : available)
            {
                if (detail::nameOf(props) == name)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                missing.push_back(name);
            }
        }

        #ifdef SUNFLOWER_DEBUG
        if (!missing.empty())
        {
            spdlog::warn("Missing Vulkan feature(s): {}", fmt::join(missing, ", "));
        }
        #endif

        return missing.empty();
    }

    [[nodiscard]] constexpr vk::Format toVulkan(const Format format)
    {
        using enum Format;
        using enum vk::Format;
        switch (format)
        {
            case None:          return eUndefined;;
            case RGBA8_Unorm:   return eR8G8B8A8Unorm;
            case RGBA8_Srgb:    return eR8G8B8A8Srgb;
            case BGRA8_Unorm:   return eB8G8R8A8Unorm;
            case BGRA8_Srgb:    return eB8G8R8A8Srgb;
            case R32_Float:     return eR32Sfloat;
            case RG32_Float:    return eR32G32Sfloat;
            case RGBA16_Float:  return eR16G16B16A16Sfloat;
            case RGBA32_Float:  return eR32G32B32A32Sfloat;
            case D32_Float:     return eD32Sfloat;
        }
        assert(false && "Unknown Format");
        std::abort();
    }

    [[nodiscard]] constexpr vk::ImageUsageFlags toVulkan(const TextureUsage textureUsage)
    {
        return {};
    }

    [[nodiscard]] constexpr vk::ImageType toVulkan_ImageType(const TextureType textureType)
    {
        using enum TextureType;
        switch (textureType)
        {
            case e1D:       return vk::ImageType::e1D;
            case e2D:       return vk::ImageType::e2D;
            case e3D:       return vk::ImageType::e3D;
            case eCube:     return vk::ImageType::e2D;
        }
        assert(false && "Unknown TextureType");
        std::abort();
    }

    [[nodiscard]] constexpr vk::ImageViewType toVulkan_ImageViewType(const TextureType textureType)
    {
        using enum TextureType;
        switch (textureType)
        {
            case e1D:       return vk::ImageViewType::e1D;
            case e2D:       return vk::ImageViewType::e2D;;
            case e3D:       return vk::ImageViewType::e3D;
            case eCube:     return vk::ImageViewType::eCube;
        }
        assert(false && "Unknown TextureType");
        std::abort();
    }
}
