#include "VertexTypes.hpp"

namespace nbl
{
    RHI::VertexAttributes Vertex::getAttributes(const uint32_t firstLoc, const uint32_t binding)
    {
        return {{ firstLoc + 0, binding, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) }};
    }

    RHI::VertexBinding Vertex::getBinding(const uint32_t binding)
    {
        return { binding, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    uint32_t Vertex::getAttributeCount() noexcept
    {
        return 1;
    }

    RHI::VertexAttributes VertexAttributes::getAttributes(const uint32_t firstLoc, const uint32_t binding)
    {
        return {
            { firstLoc + 0, binding, vk::Format::eR32G32B32Sfloat, offsetof(VertexAttributes, normal  ) },
            { firstLoc + 1, binding, vk::Format::eR32G32B32Sfloat, offsetof(VertexAttributes, tangent ) },
            { firstLoc + 2, binding, vk::Format::eR32G32Sfloat,    offsetof(VertexAttributes, texcoord) },
        };
    }

    RHI::VertexBinding VertexAttributes::getBinding(const uint32_t binding)
    {
        return { binding, sizeof(VertexAttributes), vk::VertexInputRate::eVertex };
    }

    uint32_t VertexAttributes::getAttributeCount() noexcept
    {
        return 3;
    }
}
