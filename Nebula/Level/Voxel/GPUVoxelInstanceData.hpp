#pragma once

#include <glm/glm.hpp>

#include "VulkanRHI/Rendering/VertexTraits.hpp"

struct GPUVoxelInstanceData
{
    glm::mat4x4 model;
    glm::vec4   color;

    [[nodiscard]] static std::vector<vk::VertexInputAttributeDescription> getAttributes(const uint32_t firstLoc = 0, const uint32_t binding = 0) noexcept
    {
        return {
            { firstLoc + 0, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
            { firstLoc + 1, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
            { firstLoc + 2, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
            { firstLoc + 3, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
            { firstLoc + 4, binding, vk::Format::eR32G32B32A32Sfloat, offsetof(GPUVoxelInstanceData, color) },
        };
    }

    [[nodiscard]] static vk::VertexInputBindingDescription getBinding(const uint32_t binding) noexcept
    {
        return { binding, sizeof(GPUVoxelInstanceData), vk::VertexInputRate::eInstance };
    }

    [[nodiscard]] static uint32_t getAttributeCount() noexcept
    {
        return 6;
    }
};
