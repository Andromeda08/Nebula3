#pragma once

#include <format>
#include <set>
#include <vector>

#include "RenderGraph.hpp"
#include "RenderGraphTraits.hpp"

namespace rg
{
    struct RenderGraphContextCreateInfo
    {
        RHIFeatureLevel rhiFeatureLevel;
    };

    class RenderGraphContext
    {
    public:
        nbl_DISABLE_COPY(RenderGraphContext)
        nbl_CTOR_SHARED(RenderGraphContext);

        const std::set<NodeType>& getEnabledNodes() const noexcept
        {
            return mEnabledNodeTypes;
        }

        const std::vector<UPtr<RenderGraph>>& getRenderGraphs() const
        {
            return mRenderGraphs;
        }

        void setActiveEditorRenderGraph(RenderGraph* pRenderGraph)
        {
            mActiveEditorGraph = pRenderGraph;
        }

        RenderGraph* getActiveEditorRenderGraph() const
        {
            return mActiveEditorGraph;
        }

        RenderGraph* createRenderGraph()
        {
            const auto name = std::format("RenderGraph #{}", mRenderGraphs.size());
            mRenderGraphs.push_back(std::move(RenderGraph::create({ name })));
            mActiveEditorGraph = mRenderGraphs.back().get();
            return mActiveEditorGraph;
        }

    private:
        // Configuration
        std::set<NodeType>              mEnabledNodeTypes;

        // RenderGraphs
        std::vector<UPtr<RenderGraph>>  mRenderGraphs;
        RenderGraph*                    mActiveEditorGraph = nullptr;

        // RenderPath
    };
}
