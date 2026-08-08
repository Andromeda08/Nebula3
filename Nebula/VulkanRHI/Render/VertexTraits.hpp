#pragma once

#include <concepts>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    template <typename T>
    concept VertexType = requires (T t, uint32_t u, uint32_t b) {
        { T::getAttributes(u, b) } -> std::same_as<std::vector<vk::VertexInputAttributeDescription>>;
        { T::getBinding(u)       } -> std::same_as<vk::VertexInputBindingDescription>;
        { T::getAttributeCount() } -> std::same_as<uint32_t>;
    };

    template <typename T>
    concept IndexType = std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>;
}
