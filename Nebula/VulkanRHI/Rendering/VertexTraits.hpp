#pragma once

#include <concepts>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    using VertexAttributes = std::vector<vk::VertexInputAttributeDescription>;
    using VertexBinding    = vk::VertexInputBindingDescription;

    template <typename T>
    concept VertexType = requires (T t, uint32_t u) {
        { T::getAttributes(u, u) } -> std::same_as<VertexAttributes>;
        { T::getBinding(u) } -> std::same_as<VertexBinding>;
        { T::getAttributeCount() } -> std::same_as<uint32_t>;
    };
}
