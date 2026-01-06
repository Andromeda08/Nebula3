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

    // [AddNode-1] Add to enum type
    #define NODE_TYPES(X) \
        X(Unknown) \
        X(Scene) \
        X(Present) \
        X(HelloTrianglePresent) \
        X(GBufferPass) \
        X(LightingPass) \
        X(AmbientOcclusionPass) \
        X(AntiAliasingPass) \
        X(CombinePass)

    #pragma region "NodeType : Enum, JSON Serialization, toString"
    enum class NodeType
    {
        #define X(name) name,
        NODE_TYPES(X)
        #undef X
    };

    #define X(name) { NodeType::name, #name },
    NLOHMANN_JSON_SERIALIZE_ENUM(NodeType, {
        NODE_TYPES(X)
    });
    #undef X

    inline std::string toString(const NodeType nodeType) noexcept
    {
        #define X(name) case NodeType::name: return #name;
        switch (nodeType)
        {
            NODE_TYPES(X)
            default: return "Unknown";
        }
        #undef X
    }
    #pragma endregion

    inline std::set<NodeType> getAllNodeTypes() noexcept
    {
        using enum NodeType;
        #define X(name) name,
        return { NODE_TYPES(X) };
        #undef X
    }

    constexpr RHIFeatureLevel getNodeRequiredFeatureLevel(const NodeType nodeType) noexcept
    {
        using enum NodeType;
        switch (nodeType)
        {
            // [RHI Feature Level : Complete]
            case AmbientOcclusionPass:
                return RHIFeatureLevel::Complete;

            // [RHI Feature Level : Basic]
            case Scene:
            case Present:
            case HelloTrianglePresent:
            case GBufferPass:
            case LightingPass:
            case AntiAliasingPass:
            case CombinePass:
            default: {
                return RHIFeatureLevel::Basic;
            }
        }
    }

    constexpr bool isSourceNode(const NodeType nodeType) noexcept
    {
        return nodeType == NodeType::Scene;
    }

    constexpr bool isSinkNode(const NodeType nodeType) noexcept
    {
        return nodeType == NodeType::Present
            || nodeType == NodeType::HelloTrianglePresent;
    }

    #undef NODE_TYPES

    #define RESOURCE_TYPES(X) \
        X(Unknown) \
        X(SceneData) \
        X(Buffer) \
        X(Image) \
        X(TopLevelAS)

    #pragma region "ResourceType : Enum, JSON Serialization, toString"
    enum class ResourceType
    {
        #define X(name) name,
        RESOURCE_TYPES(X)
        #undef X
    };

    #define X(name) { ResourceType::name, #name },
    NLOHMANN_JSON_SERIALIZE_ENUM(ResourceType, {
        RESOURCE_TYPES(X)
    });
    #undef X

    inline std::string toString(const ResourceType nodeType) noexcept
    {
        #define X(name) case ResourceType::name: return #name;
        switch (nodeType)
        {
            RESOURCE_TYPES(X)
            default: return "Unknown";
        }
        #undef X
    }
    #pragma endregion

    inline std::set<ResourceType> getAllResourceTypes() noexcept
    {
        using enum ResourceType;
        #define X(name) name,
        return { RESOURCE_TYPES(X) };
        #undef X
    }

    #undef RESOURCE_TYPES
}
