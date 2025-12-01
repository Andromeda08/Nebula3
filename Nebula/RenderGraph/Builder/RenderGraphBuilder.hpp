#pragma once

#include <expected>
#include <string>

#include "RenderGraph/RenderGraphTraits.hpp"
#include "RenderGraph/Compiler/RenderGraphCompiler.hpp"

namespace rg
{
    class Node;
    class RenderGraph;

    class RenderGraphBuilder
    {
    public:
        explicit RenderGraphBuilder(RenderGraph* pRenderGraph);

        Node* addNode(NodeType nodeType) const noexcept;

        [[nodiscard]] std::expected<int32_t, bool> addEdge(Node* pSrc, const std::string& srcDependency, Node* pDst, const std::string& dstDependency) const noexcept;

        bool eraseNode(int32_t id) const noexcept;

        bool eraseEdge(int32_t id) const noexcept;

        void setName(const std::string& name) const noexcept;

        RenderGraphCompilerResult compile() const noexcept;

    private:
        RenderGraph*             mRenderGraph;
    };
}
