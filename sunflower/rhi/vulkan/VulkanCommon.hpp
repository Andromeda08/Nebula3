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
}
