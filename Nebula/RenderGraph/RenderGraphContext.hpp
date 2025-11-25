#pragma once

#include <set>

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

        RenderGraph* getActiveEditorRenderGraph() const
        {
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
