#include "Export.hpp"

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "RenderGraph/RenderGraphContext.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Range, start, end);

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

    void Export::mermaid_renderGraph(bool resourceNodes) noexcept
    {
    }
}
