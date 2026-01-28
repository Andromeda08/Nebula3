#include "RenderGraphBuilder.hpp"

#include "RenderGraph/Node.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/RenderGraphContext.hpp"

namespace rg
{
    RenderGraphBuilder::RenderGraphBuilder(RenderGraph* pRenderGraph)
    : mRenderGraph(pRenderGraph)
    {
    }

    Node* RenderGraphBuilder::addNode(const NodeType nodeType) const noexcept
    {
        const auto nodeInfo = getNodeCreateInfo(nodeType);
        return mRenderGraph->addNode(nodeInfo);
    }

    std::expected<int32_t, bool> RenderGraphBuilder::addEdge(Node* pSrc, const std::string& srcDependency, Node* pDst, const std::string& dstDependency) const noexcept
    {
        if (mRenderGraph->addEdge(pSrc, srcDependency, pDst, dstDependency))
        {
            const auto it = std::ranges::find_if(mRenderGraph->getEdges(), [&](const Edge& edge) -> bool {
                return edge.srcDependencyId == pSrc->getDependencyInfo(srcDependency).id
                    && edge.dstDependencyId == pDst->getDependencyInfo(dstDependency).id;
            });
            return it->id;
        }
        return std::unexpected(false);
    }

    bool RenderGraphBuilder::eraseNode(const int32_t id) const noexcept
    {
        return mRenderGraph->eraseNode(id);
    }

    bool RenderGraphBuilder::eraseEdge(const int32_t id) const noexcept
    {
        return mRenderGraph->eraseEdge(id);
    }

    void RenderGraphBuilder::setName(const std::string& name) const noexcept
    {
        mRenderGraph->setName(name);
    }

    RenderGraphCompilerResult RenderGraphBuilder::compile() const noexcept
    {
        return RenderGraphContext::compileRenderGraph(mRenderGraph);
    }
}
