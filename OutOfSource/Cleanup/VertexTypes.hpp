#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Rendering/VertexTraits.hpp"

namespace nbl
{
    // Index Type
    using IndexType = uint32_t;

    // Position
    struct Vertex
    {
        glm::vec3 position;     // RGB32 Float

        [[nodiscard]] static RHI::VertexAttributes getAttributes(uint32_t firstLoc = 0, uint32_t binding = 0);
        [[nodiscard]] static RHI::VertexBinding    getBinding(uint32_t binding);
        [[nodiscard]] static uint32_t              getAttributeCount() noexcept;
    };

    // Normal [f32vec4], Tangent [f32vec4], Texcoord [f32vec2]
    struct VertexAttributes
    {
        glm::vec4 normal;   // RGB8  SNorm, w unused
        glm::vec4 tangent;  // RGBA8 SNorm, w stores Bitangent sign
        glm::vec2 texcoord; // RG32  Float

        [[nodiscard]] static RHI::VertexAttributes getAttributes(uint32_t firstLoc = 0, uint32_t binding = 0);
        [[nodiscard]] static RHI::VertexBinding    getBinding(uint32_t binding);
        [[nodiscard]] static uint32_t              getAttributeCount() noexcept;
    };

    // Both types should satisfy the RHI VertexType concept
    static_assert(RHI::VertexType<Vertex>);
    static_assert(RHI::VertexType<VertexAttributes>);
}
