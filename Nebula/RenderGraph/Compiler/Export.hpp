#pragma once

#include <expected>

#include "RenderGraphCompiler.hpp"
#include "ResourceOptimizer.hpp"
#include "RenderGraph/RenderGraph.hpp"

namespace rg
{
    class Export
    {
    public:
        static void json_compilerResult(const RenderGraphCompilerResult& result) noexcept;

        static void json_resourceOptimizer(const ResourceOptimizerResult& result) noexcept;

        static void mermaid_renderGraph(bool resourceNodes = false) noexcept;
    };
}
