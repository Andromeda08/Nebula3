#include "RenderGraph.hpp"

#include <print>

#include "RenderPass/RenderPass.hpp"
#include "RenderPass/Molecule/SDFComputePass.hpp"

namespace rg
{
    int32_t RenderGraph::sIdSequence = 0;
}

namespace rg
{
    // [AddNode-2] Add to getNodeCreateInfo
    NodeCreateInfo getNodeCreateInfo(const NodeType nodeType)
    {
        using enum NodeType;
        switch (nodeType)
        {
            case Scene:                 return ScenePass::getNodeInfo();
            case Present:               return PresentPass::getNodeInfo();
            case HelloTrianglePresent:  return HelloTrianglePass::getNodeInfo();
            // case GBufferPass:           return GBufferPass::getNodeInfo();
            case LightingPass:          return LightingPass::getNodeInfo();
            case AmbientOcclusionPass:  return AmbientOcclusionPass::getNodeInfo();
            case AntiAliasingPass:      return AntiAliasingPass::getNodeInfo();
            case CombinePass:           return CombinePass::getNodeInfo();
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

        mNodes.push_back(std::move(makeUnique<Node>(nodeCreateInfo)));
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

        if (isSourceNode(nodeType))
        {
            mHasSourceNode = false;
        }
        if (isSinkNode(nodeType))
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
            .srcId = startNodeId,
            .dstId = endNodeId,
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

    Node* RenderGraph::getRootNode() const
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

    std::expected<nlohmann::json, std::string> RenderGraph::serializeRenderGraph() const
    {
        if (mNodes.empty())
        {
            return std::unexpected("Cannot save RenderGraph with no nodes.");
        }

        const auto nodes = mNodes
            | std::views::transform([](const UPtr<Node>& node) -> Node* { return node.get(); })
            | std::ranges::to<std::vector<Node*>>();

        nlohmann::json json;
        json["name"] = mName;
        json["edges"] = mEdges;
        json["nodes"] = nodes;
        return json;
    }

    std::expected<UPtr<RenderGraph>, std::string> RenderGraph::deserializeRenderGraph(const nlohmann::json& json, const std::set<NodeType>& supportedNodes)
    {
        auto renderGraph = RenderGraph::create({
            .name = json.at("name"),
        });

        // 1. Load nodes and generate new IDs
        std::map<int32_t, int32_t> nodeIdMap;   // Old ID -> New ID
        for (const auto& nodeJson : json.at("nodes"))
        {
            const auto newId = RenderGraph::nextId();
            nodeIdMap[nodeJson.at("id")] = newId;

            auto newNode = Node::createFromJson(nodeJson, newId);
            newNode->setGridPos(ImVec2(nodeJson.at("gridPos").at(0), nodeJson.at("gridPos").at(1)));

            renderGraph->mNodes.push_back(std::move(newNode));
        }

        // 2. Generated new IDs for node dependencies (and load styles)
        std::map<int32_t, int32_t> dependencyIdMap;  // Old ID -> New ID
        for (const auto& node : renderGraph->mNodes)
        {
            for (auto& dependency : node->getDependencies())
            {
                const auto newId = RenderGraph::nextId();
                dependencyIdMap[dependency.id] = newId;
                dependency.id = newId;
                dependency.style = Configuration::getConfig().renderGraph.getResourceStyle(dependency.resourceType);
            }
        }

        // 2. Load edges then generate new edges from new IDs.
        const std::vector<Edge> oldEdges = json.at("edges");
        for (const auto& edge : oldEdges)
        {
            const auto result = renderGraph->addEdge(
                nodeIdMap[edge.srcId], dependencyIdMap[edge.srcDependencyId],
                nodeIdMap[edge.dstId], dependencyIdMap[edge.dstDependencyId]);
            if (!result)
            {
                return std::unexpected(std::format("Failed to load edge with ID={}", edge.id));
            }
        }

        // 4. Recover edges between nodes
        for (const auto& edge : renderGraph->mEdges)
        {
            const auto result = Node::makeDirectedEdge(edge.pSrc, edge.pDst);
            if (!result)
            {
                return std::unexpected(std::format("Failed to recover underlying edge for RenderGraph edge with ID={}", edge.id));
            }
        }

        // 5. Check feature level support & set has sink and source node
        for (const auto& node : renderGraph->mNodes)
        {
            const auto nodeType = node->getNodeType();

            // Feature Level
            if (!supportedNodes.contains(nodeType))
            {
                renderGraph->mSupportsCurrentPlatform = false;
            }

            // Source & Sink
            if (isSourceNode(nodeType))
            {
                renderGraph->mHasSourceNode = true;
            }
            if (isSinkNode(nodeType))
            {
                renderGraph->mHasSinkNode = true;
            }
        }

        return renderGraph;
    }
}
