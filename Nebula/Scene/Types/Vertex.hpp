#pragma once

#include <glm/glm.hpp>
#include "VulkanRHI/Rendering/VertexTraits.hpp"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal    = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 uv        = glm::vec2(0.0f, 0.0f);
    glm::vec2 uv1       = glm::vec2(0.0f, 0.0f);
    glm::vec4 tangent   = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    [[nodiscard]] static RHI::VertexAttributes getAttributes(const uint32_t firstLoc = 0, const uint32_t binding = 0) noexcept
    {
        return {
            { firstLoc + 0, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) },
            { firstLoc + 1, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
            { firstLoc + 2, binding, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) },
            { firstLoc + 3, binding, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv1) },
            { firstLoc + 4, binding, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent) },
        };
    }

    [[nodiscard]] static RHI::VertexBinding getBinding(const uint32_t binding) noexcept
    {
        return { binding, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    [[nodiscard]] static uint32_t getAttributeCount() noexcept
    {
        return 5;
    }
};