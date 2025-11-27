#pragma once

#include <optional>
#include <set>
#include <string>
#include "Core/Types.hpp"
#include "RenderGraph/Node.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/RenderGraphTraits.hpp"

namespace rg
{
    struct ConsumerInfo
    {
        Node*           pNode;
        int32_t         consumeIdx;
        int32_t         depId;
        std::string     depName;
    };

    struct ResourceInfo
    {
        Node*                       pNode;
        int32_t                     consumeIdx;
        int32_t                     originalDepId;
        std::string                 originalDepName;
        ResourceType                resourceType;
        DependencyType              initDepType;
        bool                        isOptimizable;
        std::vector<ConsumerInfo>   consumers;
    };

    [[nodiscard]] ResourceInfo makeResourceInfo(Node* pNode, const DependencyInfo& dependencyInfo, int32_t idx);

    struct UsagePoint
    {
        int32_t         point;
        int32_t         userDependencyId = -1;
        std::string     usedAs;
        int32_t         userNodeId       = -1;
        std::string     usedBy;
        DependencyType  dependencyType   = DependencyType::Ignored;

        [[nodiscard]] static UsagePoint fromResourceInfo(const ResourceInfo& resourceInfo) noexcept;
        [[nodiscard]] static UsagePoint fromConsumerInfo(const ConsumerInfo& consumerInfo) noexcept;
    };

    inline bool operator<(const UsagePoint& lhs, const UsagePoint& rhs)
    {
        return lhs.point < rhs.point;
    }

    inline bool operator==(const UsagePoint& lhs, const UsagePoint& rhs)
    {
        return lhs.point == rhs.point;
    }

    [[nodiscard]] Range makeRange(const std::set<UsagePoint>& usagePoints) noexcept;

    [[nodiscard]] std::set<UsagePoint> getUsagePoints(const ResourceInfo& resourceInfo);

    struct OptimizerResource
    {
        int32_t              id;
        std::set<UsagePoint> usagePoints;
        ResourceInfo         originalResource;
        ResourceType         resourceType;
        std::vector<Range>   usageRanges;

        [[nodiscard]] Range getUsageRange() const noexcept;

        [[nodiscard]] std::optional<UsagePoint> getUsagePoint(int32_t point) const noexcept;

        bool insertUsagePoints(const std::set<UsagePoint>& inserts) noexcept;
    };

    struct ResourceOptimizerParams
    {
        std::vector<Node*> orderedNodes;
        std::vector<Edge>  edges;
    };

    struct ResourceOptimizerResultMeta
    {
        std::vector<ResourceInfo>   originalResources   = {};
        int32_t                     nNonOptimizable     = 0;
        int32_t                     nReduction          = 0;
        int32_t                     nOriginalCount      = 0;
        Range                       timelineRange       = Range(0, 0);
        std::vector<std::string>    messages            = {};
    };

    struct ResourceOptimizerResult
    {
        // Generated resources
        std::vector<OptimizerResource> resources;
        // Metadata
        ResourceOptimizerResultMeta    meta;
    };

    class ResourceOptimizer
    {
    public:
        static ResourceOptimizerResult execute(const ResourceOptimizerParams& params);
    };
}
