#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Math/BoundingBox.hpp"

#include "VulkanRHI/Rendering/VertexTraits.hpp"

struct UIVertex
{
    glm::vec2 pos;
    glm::vec2 uv;

    [[nodiscard]] static std::vector<vk::VertexInputAttributeDescription> getAttributes(const uint32_t firstLoc = 0, const uint32_t binding = 0) noexcept
    {
        return {
                    { firstLoc + 0, binding, vk::Format::eR32G32Sfloat, offsetof(UIVertex, pos) },
                    { firstLoc + 1, binding, vk::Format::eR32G32Sfloat, offsetof(UIVertex, uv)  },
                };
    }

    [[nodiscard]] static vk::VertexInputBindingDescription getBinding(const uint32_t binding) noexcept
    {
        return { binding, sizeof(UIVertex), vk::VertexInputRate::eVertex };
    }

    [[nodiscard]] static uint32_t getAttributeCount() noexcept
    {
        return 2;
    }
};

struct UIGeometry
{
    std::vector<UIVertex>   vertices;
    std::vector<uint32_t>   indices;
    nbl::BoundingBox        bounds;
};
