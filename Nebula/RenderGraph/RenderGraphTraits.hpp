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
        HelloTrianglePresent,

        GBufferPass,
        LightingPass,
        AmbientOcclusionPass,
        AntiAliasingPass,
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(NodeType, {
        { NodeType::Unknown,              "Unknown"              },
        { NodeType::Scene,                "Scene"                },
        { NodeType::Present,              "Present"              },
        { NodeType::HelloTrianglePresent, "HelloTrianglePresent" },
        { NodeType::GBufferPass,          "GBufferPass"          },
        { NodeType::LightingPass,         "LightingPass"         },
        { NodeType::AmbientOcclusionPass, "AmbientOcclusionPass" },
        { NodeType::AntiAliasingPass,     "AntiAliasingPass"     },
    });

    inline std::string toString(const NodeType nodeType) noexcept
    {
        using enum NodeType;
        switch (nodeType)
        {
            case Scene:                 return "Scene";
            case Present:               return "Present";
            case HelloTrianglePresent:  return "HelloTrianglePresent";
            case GBufferPass:           return "GBufferPass";
            case LightingPass:          return "LightingPass";
            case AmbientOcclusionPass:  return "AmbientOcclusionPass";
            case AntiAliasingPass:      return "AntiAliasingPass";
            default:                    return "Unknown";
        }
    }

    constexpr std::set<NodeType> getAllNodeTypes() noexcept
    {
        using enum NodeType;
        return { Unknown, Scene, Present, HelloTrianglePresent, GBufferPass, LightingPass, AmbientOcclusionPass, AntiAliasingPass };
    }

    constexpr RHIFeatureLevel getNodeRequiredFeatureLevel(const NodeType nodeType) noexcept
    {
        using enum NodeType;
        switch (nodeType)
        {
            // [RHI Feature Level : Complete]
            case AmbientOcclusionPass:

            // [RHI Feature Level : Basic]
            case Unknown:
            case Scene:
            case Present:
            case HelloTrianglePresent:
            case GBufferPass:
            case LightingPass:
            case AntiAliasingPass:
            default: {
                return RHIFeatureLevel::Basic;
            }
        }
    }

    enum class ResourceType
    {
        Unknown,
        SceneData,
        Buffer,
        Image,
        TopLevelAS,
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(ResourceType, {
        { ResourceType::Unknown,    "Unknown"    },
        { ResourceType::SceneData,  "SceneData"  },
        { ResourceType::Buffer,     "Buffer"     },
        { ResourceType::Image,      "Image"      },
        { ResourceType::TopLevelAS, "TopLevelAS" },
    });

    inline std::string toString(const ResourceType nodeType) noexcept
    {
        using enum ResourceType;
        switch (nodeType)
        {
            case SceneData:     return "SceneData";
            case Buffer:        return "Buffer";
            case Image:         return "Image";
            case TopLevelAS:    return "TopLevelAS";
            default:            return "Unknown";
        }
    }
}