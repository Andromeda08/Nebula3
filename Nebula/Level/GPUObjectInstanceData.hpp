#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include "VulkanRHI/Rendering/VertexTraits.hpp"

struct GPUObjectInstanceData
{
    glm::mat4 model        = glm::mat4(1.0f);
    glm::vec4 solidColor   = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    int32_t   textureIndex = -1;

    [[nodiscard]] static RHI::VertexAttributes getAttributes(const uint32_t firstLoc = 0, const uint32_t binding = 0) noexcept
    {
        return {
            { firstLoc + 0, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
            { firstLoc + 1, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
            { firstLoc + 2, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
            { firstLoc + 3, binding, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
            { firstLoc + 4, binding, vk::Format::eR32G32B32A32Sfloat, offsetof(GPUObjectInstanceData, solidColor) },
            { firstLoc + 5, binding, vk::Format::eR32Sint, offsetof(GPUObjectInstanceData, textureIndex) },
        };
    }

    [[nodiscard]] static RHI::VertexBinding getBinding(const uint32_t binding) noexcept
    {
        return { binding, sizeof(GPUObjectInstanceData), vk::VertexInputRate::eInstance };
    }


    [[nodiscard]] static uint32_t getAttributeCount() noexcept
    {
        return 6;
    }
};
