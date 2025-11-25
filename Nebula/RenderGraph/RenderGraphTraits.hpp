#pragma once

#include <set>
#include <nlohmann/json.hpp>
#include "VulkanRHI/RHIConfiguration.hpp"

namespace rg
{
    enum class DependencyType
    {
        Ignored,    // Ignore dependency
        Expose,     // Resource is not read or written, but exposed by a Node (e.g. Scene data)
        Read,       // Resource is read
        Write,      // Resource is written
    };

    enum class NodeType
    {
        Unknown,
        Scene,
        Present,
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(NodeType, {
        { NodeType::Unknown, "Unknown" },
        { NodeType::Scene,   "Scene"   },
        { NodeType::Present, "Present" },
    });

    inline std::string toString(const NodeType nodeType) noexcept
    {
        using enum NodeType;
        switch (nodeType)
        {
            case Scene:     return "Scene";
            case Present:   return "Present";
            default:        return "Unknown";
        }
    }

    constexpr std::set<NodeType> getAllNodeTypes() noexcept
    {
        using enum NodeType;
        return { Unknown, Scene, Present };
    }

    constexpr RHIFeatureLevel getNodeRequiredFeatureLevel(const NodeType nodeType) noexcept
    {
        using enum NodeType;
        switch (nodeType)
        {
            // [RHI Feature Level : Complete]

            // [RHI Feature Level : Basic]
            case Unknown:
            case Scene:
            case Present:
            default: {
                return RHIFeatureLevel::Basic;
            }
        }
    }

    enum class ResourceType
    {
        Unknown,
        Buffer,
        Image,
        TopLevelAS,
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(ResourceType, {
        { ResourceType::Unknown,    "Unknown"    },
        { ResourceType::Buffer,     "Buffer"     },
        { ResourceType::Image,      "Image"      },
        { ResourceType::TopLevelAS, "TopLevelAS" },
    });
}