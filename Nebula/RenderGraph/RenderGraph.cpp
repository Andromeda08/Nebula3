#include "RenderGraph.hpp"

#include <print>

#include "RenderPass/HelloTrianglePass.hpp"
#include "RenderPass/Special/AmbientOcclusionPass.hpp"
#include "RenderPass/Special/AntiAliasingPass.hpp"
#include "RenderPass/Special/GBufferPass.hpp"
#include "RenderPass/Special/LightingPass.hpp"
#include "RenderPass/Special/PresentPass.hpp"
#include "RenderPass/Special/ScenePass.hpp"

namespace rg
{
    int32_t RenderGraph::sIdSequence = 0;
}

namespace rg
{
    NodeCreateInfo getNodeCreateInfo(const NodeType nodeType)
    {
        using enum NodeType;
        switch (nodeType)
        {
            case Scene:                 return ScenePass::getNodeInfo();
            case Present:               return PresentPass::getNodeInfo();
            case HelloTrianglePresent:  return HelloTrianglePass::getNodeInfo();
            case GBufferPass:           return GBufferPass::getNodeInfo();
            case LightingPass:          return LightingPass::getNodeInfo();
            case AmbientOcclusionPass:  return AmbientOcclusionPass::getNodeInfo();
            case AntiAliasingPass:      return AntiAliasingPass::getNodeInfo();
            default:                    throw std::runtime_error("NodeType not supported");
        }
    }

    RenderGraph::RenderGraph(const RenderGraphCreateInfo& createInfo)
    : mName(createInfo.name)
    {
    }

    Node* RenderGraph::addNode(const NodeCreateInfo& createInfo) noexcept
    {
        auto nodeCreateInfo = createInfo;
        nodeCreateInfo.nodeStyle = Configuration::getConfig().renderGraph.getNodeStyle(createInfo.nodeType);

        mNodes.push_back(std::move(makeUnique<Node>(createInfo)));
        for (auto& dependency : mNodes.back().get()->getDependencies())
        {
            dependency.id = RenderGraph::nextId();
            dependency.style = Configuration::getConfig().renderGraph.getResourceStyle(dependency.resourceType);
        }

        if (createInfo.nodeType == NodeType::Scene)
        {
            mHasSourceNode = true;
        }
        if (createInfo.nodeType == NodeType::Present || createInfo.nodeType == NodeType::HelloTrianglePresent)
        {
            mHasSinkNode = true;
        }

        return mNodes.back().get();
    }

    bool RenderGraph::eraseNode(const int32_t nodeId) noexcept
    {
        const auto it = std::ranges::find_if(mNodes, [&nodeId](const auto& node){ return node->getId() == nodeId; });
        if (it == std::end(mNodes))
        {
            std::println("[RenderGraph] Node deletion failed: invalid ID ({})", nodeId);
            return false;
        }

        std::vector<int32_t> edgesToDelete;
        for (const auto& edge : mEdges)
        {
            if (edge.pSrc->getId() == nodeId || edge.pDst->getId() == nodeId)
            {
                edgesToDelete.push_back(edge.id);
            }
        }
        for (const auto& edgeId : edgesToDelete)
        {
            if (!eraseEdge(edgeId))
            {
                std::println("Error occurred while deleting a node ({}): edge deletion failed [edgeId={}]",
                    getNode(nodeId)->getDisplayName(), edgeId);
            }
        }

        const auto nodeType = getNode(nodeId)->getNodeType();
        const auto nodeName = getNode(nodeId)->getDisplayName();

        mNodes.erase(it);

        if (nodeType == NodeType::Scene)
        {
            mHasSourceNode = false;
        }
        if (nodeType == NodeType::Present)
        {
            mHasSinkNode = false;
        }

        std::println("Deleted Node: {} [nodeId={}]", nodeName, nodeId);
        return true;
    }

    bool RenderGraph::addEdge(const int32_t startNodeId, const int32_t startAttrId, const int32_t endNodeId, const int32_t endAttrId)
    {
        auto* startNode = getNode(startNodeId);
        auto& startAttr = startNode->getDependencyInfo(startAttrId);

        auto* endNode = getNode(endNodeId);
        auto& endAttr = endNode->getDependencyInfo(endAttrId);

        const auto edgeExists = std::ranges::any_of(mEdges, [&startAttr, &endAttr](const auto& edge){
            return edge.srcDependencyId == startAttr.id && edge.dstDependencyId == endAttr.id;
        });

        if (endAttr.isConnected)
        {
            std::println(R"([RenderGraph] The attribute "{}" of "{}" already has an input attached)",
                endAttr.name, endNode->getDisplayName());
            return false;
        }

        if (edgeExists)
        {
            std::println(R"([RenderGraph] The attributes "{}" and "{}" are already connected)",
                startAttr.name, endAttr.name);
            return false;
        }

        if (startAttr.resourceType != endAttr.resourceType)
        {
            std::println(R"([RenderGraph] Type of attribute "{}" is not compatible with "{}")",
                startAttr.name, endAttr.name);
            return false;
        }

        Node::makeDirectedEdge(startNode, endNode);
        mEdges.push_back({
            .id = RenderGraph::nextId(),
            .pSrc = startNode,
            .srcDependencyId = startAttrId,
            .pDst = endNode,
            .dstDependencyId = endAttrId,
            .resourceType = startAttr.resourceType,
        });
        endAttr.isConnected = true;

        std::println(R"([RenderGraph] Connected: [{}]::({}) --> [{}]::({}))",
            startNode->getDisplayName(), startAttr.name, endNode->getDisplayName(), endAttr.name);
        return true;
    }

    bool RenderGraph::addEdge(Node* startNode, const std::string& startAttrName, Node* endNode, const std::string& endAttrName)
    {
        const auto& startAttr = startNode->getDependencyInfo(startAttrName);
        const auto& endAttr   = endNode->getDependencyInfo(endAttrName);
        return addEdge(startNode->getId(), startAttr.id, endNode->getId(), endAttr.id);
    }

    bool RenderGraph::eraseEdge(int32_t edgeId)
    {
        if (const auto edge = std::ranges::find_if(mEdges, [&edgeId](const auto& e){ return e.id == edgeId; });
            edge != std::end(mEdges))
        {
                  auto* startNode   = edge->pSrc;
            const auto& startAttrib = startNode->getDependencyInfo(edge->srcDependencyId);

            auto* endNode   = edge->pDst;
            auto& endAttrib = endNode->getDependencyInfo(edge->dstDependencyId);
            endAttrib.isConnected = false;

            Node::deleteDirectedEdge(startNode, endNode);

            mEdges.erase(edge);

            std::println("[RenderGraph] Deleted edge: [{}]::[{}] --> [{}]::[{}]",
                startNode->getDisplayName(), startAttrib.name, endNode->getDisplayName(), endAttrib.name);
            return true;
        }

        std::println("[RenderGraph] Error occurred when deleting edge: invalid ID ({})", edgeId);
        return false;
    }

    void RenderGraph::reset()
    {
        mEdges.clear();
        mNodes.clear();
        mHasSourceNode = false;
        mHasSinkNode   = false;
    }

    int32_t RenderGraph::nextId() noexcept
    {
        return sIdSequence++;
    }

    Node* RenderGraph::getNode(const int32_t id) noexcept
    {
        const auto it = std::ranges::find_if(mNodes, [&id](const auto& node){ return node->getId() == id; });
        return (it == std::end(mNodes))
             ? nullptr
             : it->get();
    }

    Node* RenderGraph::getRootNode()
    {
        for (const auto& node : mNodes)
        {
            if (node->getNodeType() == NodeType::Scene)
            {
                return node.get();
            }
        }
        return nullptr;
    }
}
