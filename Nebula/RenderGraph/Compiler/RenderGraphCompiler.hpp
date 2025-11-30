#pragma once

#include <string>
#include <vector>
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

        std::string                                  inputGraphName;
    };

    class RenderGraphCompiler
    {
    public:
        explicit RenderGraphCompiler(RenderGraph* pRenderGraph);

        RenderGraphCompilerResult compile() const noexcept;

    private:
        RenderGraph* mRenderGraph;
    };
}
