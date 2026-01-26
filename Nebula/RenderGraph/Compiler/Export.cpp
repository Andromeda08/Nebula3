#include "Export.hpp"

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "RenderGraph/RenderGraphContext.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Range, start, end);

// JSON serialization definitions
namespace rg
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConsumerInfo, consumeIdx, depId, depName);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceInfo,
        consumeIdx, originalDepId, originalDepName, resourceType,
        initDepType, isOptimizable, consumers);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UsagePoint,
        point, userDependencyId, usedAs, userNodeId, usedBy,
        dependencyInfo, dependencyInfo);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OptimizerResource,
        id, usagePoints, originalResource, resourceType, usageRanges);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceOptimizerResultMeta,
        originalResources, nNonOptimizable, nReduction, nOriginalCount,
        timelineRange, messages);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceOptimizerResult, resources, meta);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RenderGraphCompilerResult,
        success, nodeExecutionOrder, resourceTemplates, messages, optimizerResultMeta, inputGraphName);
}

// Export implementation
namespace rg
{
    void Export::json_compilerResult(const RenderGraphCompilerResult& result) noexcept
    {
        const nlohmann::json json = result;

        const auto now = std::chrono::high_resolution_clock::now();
        const std::string fileName = std::format("{}/compiled_{}_{}.json",
            RenderGraphContext::sRenderGraphExportDirectory, result.inputGraphName, now.time_since_epoch());

        std::ofstream file(fileName);
        assert(file.is_open());
        file << std::setw(4) << json << std::endl;
        file.close();
    }

    void Export::json_resourceOptimizer(const ResourceOptimizerResult& result) noexcept
    {
        const nlohmann::json json = result;

        const auto now = std::chrono::high_resolution_clock::now();
        const std::string fileName = std::format("{}/resource_optimizer_{}.json",
            RenderGraphContext::sRenderGraphExportDirectory, now.time_since_epoch());

        std::ofstream file(fileName);
        assert(file.is_open());
        file << std::setw(4) << json << std::endl;
        file.close();
    }

    namespace mermaid
    {
        [[nodiscard]] std::string colorToHex(const Color& color) noexcept
        {
            std::stringstream ss;
            ss << "#" << std::hex << std::uppercase << std::setfill('0')
               << std::setw(2) << +color[0]
               << std::setw(2) << +color[1]
               << std::setw(2) << +color[2];
            return ss.str();
        }

        [[nodiscard]] std::string makeNodeClass(const NodeType nodeType) noexcept
        {
            auto [cTitleBar, cTitleBarSpecial] = Configuration::getConfig().renderGraph.getNodeStyle(nodeType);
            return std::format("classDef style_{} color:#f5f5f5,fill:{},stroke:{},stroke-width:1px;",
                toString(nodeType), /* fill */ colorToHex(cTitleBar), /* stroke */ colorToHex(cTitleBar));
        }

        [[nodiscard]] std::string makeResourceClass(const ResourceType resourceType) noexcept
        {
            auto [cPin, cLink] = Configuration::getConfig().renderGraph.getResourceStyle(resourceType);
            return std::format("classDef style_{} color:#f5f5f5,fill:{},stroke:{},stroke-width:1px;",
                toString(resourceType), /* fill */ colorToHex(cLink), /* stroke */ colorToHex(cLink));
        }
    }

    void Export::mermaid_renderGraph(const RenderGraph* pRenderGraph, bool collapseEdges) noexcept
    {
        std::vector<std::string> output = { "flowchart TD" };

        // define style classes for all node and resource types
        std::ranges::for_each(getAllNodeTypes(), [&output](const auto& type) -> void {
            output.push_back(mermaid::makeNodeClass(type));
        });
        std::ranges::for_each(getAllResourceTypes(), [&output](const auto& type) -> void {
            output.push_back(mermaid::makeResourceClass(type));
        });

        // process graph
        for (const auto& node : pRenderGraph->getNodes())
        {
            output.push_back(std::format("{}[{}]:::style_{}",
                node->getId(), node->getDisplayName(), toString(node->getNodeType())));
        }

        for (const auto& edge : pRenderGraph->getEdges())
        {
            if (collapseEdges)
            {
                // use src dependency as resource node
                output.push_back(std::format("{}[{}]:::style_{}",
                    edge.pSrcDependency->id, edge.pSrcDependency->name, toString(edge.resourceType)));
                output.push_back(std::format("{} --> {}", edge.pSrc->getId(), edge.pSrcDependency->id));
                output.push_back(std::format("{} --> {}", edge.pSrcDependency->id, edge.pDst->getId()));
            }
            else
            {
                output.push_back(std::format("{}[{}]:::style_{}",
                edge.pSrcDependency->id, edge.pSrcDependency->name, toString(edge.resourceType)));
                output.push_back(std::format("{}[{}]:::style_{}",
                    edge.pDstDependency->id, edge.pDstDependency->name, toString(edge.resourceType)));

                output.push_back(std::format("{} --> {}", edge.pSrc->getId(), edge.pSrcDependency->id));
                output.push_back(std::format("{} --> {}", edge.pSrcDependency->id, edge.pDstDependency->id));
                output.push_back(std::format("{} --> {}", edge.pDstDependency->id, edge.pDst->getId()));
            }
        }

        const auto now = std::chrono::high_resolution_clock::now();
        const std::string fileName = std::format("{}/render_graph_{}.mermaid",
            RenderGraphContext::sRenderGraphExportDirectory, now.time_since_epoch());

        std::ofstream file(fileName);
        assert(file.is_open());
        for (const auto& s : output)
        {
            file << s << '\n';
        }
        file.close();
    }
}
