#include "RenderGraphContext.hpp"

namespace rg
{
    RenderGraphContext::RenderGraphContext(const RenderGraphContextCreateInfo& createInfo)
    {
        for (const auto nodeType : getAllNodeTypes())
        {
            if (createInfo.rhiFeatureLevel <= getNodeRequiredFeatureLevel(nodeType))
            {
                mEnabledNodeTypes.insert(nodeType);
            }
        }

        mRenderGraphs.push_back(RenderGraph::create({"Default Graph"}));
        mActiveEditorGraph = mRenderGraphs.back().get();
    }
}