#pragma once

#include <format>
#include <ranges>
#include <string>
#include <vector>
#include "GraphAlgorithm.hpp"
#include "ResourceOptimizer.hpp"
#include "RenderGraph/RenderGraph.hpp"

namespace rg
{
    struct ImageBarrier
    {
        int32_t         insertPoint;
        RHI::ImageUsage newUsage;
    };

    struct RenderGraphCompilerResult
    {
        bool                                         success;

        std::vector<Node*>                           nodeExecutionOrder;
        std::vector<OptimizerResource>               resourceTemplates;
        std::map<int32_t, std::vector<ImageBarrier>> imageBarrierTemplates;

        std::vector<std::string>                     messages;
        ResourceOptimizerResultMeta                  optimizerResultMeta;
    };

    class RenderGraphCompiler
    {
    public:
        explicit RenderGraphCompiler(RenderGraph* pRenderGraph)
        : mRenderGraph(pRenderGraph)
        {
        }

        RenderGraphCompilerResult compile() const noexcept
        {
            std::vector<std::string> messages;

            // =====================================
            // [Step 1] Cull unreachable nodes
            // =====================================
            auto* rootNode = mRenderGraph->getRootNode();
            if (!rootNode)
            {
                return { false };
            }

            std::vector<Node*> remainingNodes = BFS::execute(rootNode)
                | std::views::transform([&](const int32_t nodeId) -> Node* { return mRenderGraph->getNode(nodeId); })
                | std::ranges::to<std::vector<Node*>>();

            const auto nCulledNodes = mRenderGraph->getNodes().size() - remainingNodes.size();
            if (nCulledNodes != 0)
            {
                messages.push_back(std::format("[Step 1] Culled {} unreachable node(s).", nCulledNodes));
            }
            else
            {
                messages.push_back("[Step 1] No unreachable nodes were found.");
            }

            // =====================================
            // [Step 2] Serial execution order
            // =====================================
            const auto tsortResult = TopologicalSort::execute(remainingNodes);
            if (!tsortResult.has_value())
            {
                messages.push_back("[Step 2] Failed to determine the serial execution order: graph is not a DAG");
                return { false };
            }

            std::vector<Node*> serialExecutionOrder = tsortResult.value();

            // =====================================
            // [Step 3] Resource optimization
            // =====================================
            const auto optimizerResult = ResourceOptimizer::execute({
                .orderedNodes = serialExecutionOrder,
                .edges        = mRenderGraph->getEdges(),
            });

            // =====================================
            // [Step 4] Create Image Barriers
            // =====================================
            std::map<int32_t, std::vector<ImageBarrier>> imageBarriers;
            for (const auto& optimizerResource : optimizerResult.resources)
            {
                imageBarriers[optimizerResource.id] = {};

                if (optimizerResource.resourceType != ResourceType::Image) continue;
                for (const auto& usageRange : optimizerResource.usageRanges)
                {
                    const UsagePoint firstUsagePoint = optimizerResource.getUsagePoint(usageRange.start).value();
                    const RHI::ImageUsage newUsage = mRenderGraph->getNode(firstUsagePoint.userNodeId)->getDependencyInfo(firstUsagePoint.userDependencyId).imageUsage;
                    imageBarriers[optimizerResource.id].push_back({ firstUsagePoint.point, newUsage });
                }
            }

            for (const auto& message : optimizerResult.meta.messages)
            {
                messages.push_back(message);
            }

            return {
                .success                = true,
                .nodeExecutionOrder     = serialExecutionOrder,
                .resourceTemplates      = optimizerResult.resources,
                .imageBarrierTemplates  = imageBarriers,
                .messages               = messages,
                .optimizerResultMeta    = optimizerResult.meta,
            };
        }

    private:
        RenderGraph* mRenderGraph;
    };
}
