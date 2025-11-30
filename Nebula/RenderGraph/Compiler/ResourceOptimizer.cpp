#include "ResourceOptimizer.hpp"

#include <algorithm>
#include <format>
#include "Core/ToString.hpp"

namespace rg
{
    #pragma region "Util impl"

    ResourceInfo makeResourceInfo(Node* pNode, const DependencyInfo& dependencyInfo, const int32_t idx)
    {
        return {
            .pNode           = pNode,
            .consumeIdx      = idx,
            .originalDepId   = dependencyInfo.id,
            .originalDepName = dependencyInfo.name,
            .resourceType    = dependencyInfo.resourceType,
            .initDepType     = dependencyInfo.dependencyType,
            .isOptimizable   = dependencyInfo.resourceType == ResourceType::Image && !dependencyInfo.dontOptimize,
            .consumers       = {},
        };
    }

    UsagePoint UsagePoint::fromResourceInfo(const ResourceInfo& resourceInfo) noexcept
    {
        const auto dependencyInfo = resourceInfo.pNode->getDependencyInfo(resourceInfo.originalDepId);
        return {
            .point            = resourceInfo.consumeIdx,
            .userDependencyId = resourceInfo.originalDepId,
            .usedAs           = resourceInfo.originalDepName,
            .userNodeId       = resourceInfo.pNode->getId(),
            .usedBy           = resourceInfo.pNode->getDisplayName(),
            .dependencyType   = resourceInfo.initDepType,
            .dependencyInfo   = dependencyInfo,
        };
    }

    UsagePoint UsagePoint::fromConsumerInfo(const ConsumerInfo& consumerInfo) noexcept
    {
        const auto dependencyInfo = consumerInfo.pNode->getDependencyInfo(consumerInfo.depId);
        return {
            .point            = consumerInfo.consumeIdx,
            .userDependencyId = consumerInfo.depId,
            .usedAs           = consumerInfo.depName,
            .userNodeId       = consumerInfo.pNode->getId(),
            .usedBy           = consumerInfo.pNode->getDisplayName(),
            .dependencyType   = dependencyInfo.dependencyType,
            .dependencyInfo   = dependencyInfo,
        };
    }

    Range makeRange(const std::set<UsagePoint>& usagePoints) noexcept
    {
        const auto min = std::min_element(usagePoints.begin(), usagePoints.end());
        const auto max = std::max_element(usagePoints.begin(), usagePoints.end());
        return Range(min->point, max->point);
    }

    std::set<UsagePoint> getUsagePoints(const ResourceInfo& resourceInfo)
    {
        std::set<UsagePoint> usagePoints = { UsagePoint::fromResourceInfo(resourceInfo) };
        for (const auto consumer : resourceInfo.consumers)
        {
            usagePoints.insert(UsagePoint::fromConsumerInfo(consumer));
        }
        return usagePoints;
    }

    Range OptimizerResource::getUsageRange() const noexcept
    {
        return makeRange(usagePoints);
    }

    std::optional<UsagePoint> OptimizerResource::getUsagePoint(const int32_t point) const noexcept
    {
        const auto it = std::ranges::find_if(usagePoints, [&point](const UsagePoint& usagePoint) -> bool { return point == usagePoint.point; });
        return (it == usagePoints.end()) ? std::nullopt : std::make_optional(*it);
    }

    std::optional<UsagePoint> OptimizerResource::getFirstUsagePoint() const noexcept
    {
        if (usagePoints.empty())
        {
            return std::nullopt;
        }
        return *std::min_element(usagePoints.begin(), usagePoints.end());
    }

    bool OptimizerResource::insertUsagePoints(const std::set<UsagePoint>& inserts) noexcept
    {
        std::vector<UsagePoint> intersection;
        std::set_intersection(std::begin(usagePoints), std::end(usagePoints), std::begin(inserts), std::end(inserts), std::back_inserter(intersection));

        if (!intersection.empty())
        {
            return false;
        }

        for (const auto& point : inserts)
        {
            usagePoints.insert(point);
        }

        return true;
    }

    #pragma endregion

    std::vector<ResourceInfo> getProducedResources(const ResourceOptimizerParams& params)
    {
        std::vector<ResourceInfo> result;

        // Find produced resources
        for (int32_t i = 0; i < params.orderedNodes.size(); i++)
        {
            for (const auto& dependency : params.orderedNodes[i]->getDependencies())
            {
                if (dependency.dependencyType == DependencyType::Read || dependency.dependencyType == DependencyType::Ignored)
                {
                    continue;
                }
                result.push_back(makeResourceInfo(params.orderedNodes[i], dependency, i));
            }
        }

        // Find consumed resources
        for (auto& resourceInfo : result)
        {
            for (const auto& edge : params.edges)
            {
                if (resourceInfo.pNode->getId() != edge.pSrc->getId())   continue;
                if (resourceInfo.pNode->getId() == edge.pDst->getId())   continue;
                if (resourceInfo.originalDepId  != edge.srcDependencyId) continue;

                int32_t consumerIdx = 0;
                for (const auto* node : params.orderedNodes)
                {
                    if (node->getId() == edge.pDst->getId())
                    {
                        break;
                    }
                    consumerIdx++;
                }

                ConsumerInfo consumerInfo = {
                    .pNode      = edge.pDst,
                    .consumeIdx = consumerIdx,
                    .depId      = edge.dstDependencyId,
                    .depName    = edge.pDst->getDependencyInfo(edge.dstDependencyId).name,
                };
                resourceInfo.consumers.push_back(consumerInfo);
            }
        }

        return result;
    }

    ResourceOptimizerResult ResourceOptimizer::execute(const ResourceOptimizerParams& params)
    {
        ResourceOptimizerResult result;

        result.meta.messages.push_back("[RenderGraph] ================={ Resource Optimizer Start }=================");

        const std::vector<ResourceInfo> R = getProducedResources(params);

        for (const auto r : R)
        {
            const auto initialUsagePoints = getUsagePoints(r);
            OptimizerResource resource = {
                .id               = RenderGraph::nextId(),
                .usagePoints      = initialUsagePoints,
                .originalResource = r,
                .resourceType     = r.resourceType,
                .usageRanges      = { makeRange(initialUsagePoints) },
            };

            // [Case 1] No resources yet
            // [Case 2] Resource is non-optimizable
            if (result.resources.empty() || !r.isOptimizable)
            {
                result.resources.push_back(resource);
                result.meta.nNonOptimizable++;
                result.meta.messages.push_back(std::format(
                    "[RenderGraph] Created new resource [id={}, type={}, opt={}]",
                    resource.id, toString(resource.resourceType), r.isOptimizable ? "yes" : "no"));
                continue;
            }

            // [Case 3] Try to insert into existing resource
            bool wasInserted = false;
            for (auto& optimizerResource : result.resources)
            {
                Range currentRange  = optimizerResource.getUsageRange();
                Range incomingRange = makeRange(resource.usagePoints);

                const bool canInsert = !currentRange.overlaps(incomingRange) && r.resourceType == optimizerResource.resourceType;
                if (canInsert)
                {
                    wasInserted = optimizerResource.insertUsagePoints(resource.usagePoints);
                    if (wasInserted)
                    {
                        resource.usageRanges.push_back(incomingRange);
                        result.meta.nReduction++;
                        result.meta.messages.push_back(std::format(
                            "[RenderGraph] Resource [id={}, type={}] was reused in range [{}, {}] with {} usage points.",
                            optimizerResource.id, toString(optimizerResource.resourceType),
                            std::min_element(resource.usagePoints.begin(), resource.usagePoints.end())->point,
                            std::max_element(resource.usagePoints.begin(), resource.usagePoints.end())->point,
                            resource.usagePoints.size()));
                        break;
                    }
                }
            }

            if (!wasInserted)
            {
                result.resources.push_back(resource);
                result.meta.messages.push_back(std::format(
                    "[RenderGraph] Created new resource (could not be optimized) [id={}, type={}]",
                    resource.id, toString(resource.resourceType)));
            }
        }

        result.meta.originalResources = R;
        result.meta.nOriginalCount    = static_cast<int32_t>(R.size());
        result.meta.timelineRange     = Range(0, static_cast<int32_t>(params.orderedNodes.size() - 1));

        result.meta.messages.push_back("[RenderGraph] =================={ Resource Optimizer End }==================");

        return result;
    }
}
