#pragma once

#include <string>

#include "RenderGraphTraits.hpp"
#include "RGConfiguration.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace rg
{
    struct DependencyInfo
    {
        std::int32_t         id              = -1;
        std::string          name            = "Unknown Resource";
        DependencyType       dependencyType  = DependencyType::Ignored;
        ResourceType         resourceType    = ResourceType::Unknown;
        bool                 isConnected     = false;
        const ResourceStyle& style;

        // ❗If resourceType is Image
        RHI::ImageUsage      imageUsage      = RHI::ImageUsage::Undefined;
    };
}
