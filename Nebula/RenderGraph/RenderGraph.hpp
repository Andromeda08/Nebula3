#pragma once

#include <vector>
#include <nlohmann/json.hpp>

#include "Node.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace rg
{
    struct Edge
    {
        int32_t         id;
        Node*           pSrc;
        int32_t         srcDependencyId;
        Node*           pDst;
        int32_t         dstDependencyId;
        ResourceType    resourceType;

        int32_t         srcId;
        int32_t         dstId;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Edge, id, srcId, srcDependencyId, dstId, dstDependencyId, resourceType);

    struct RenderGraphCreateInfo
    {
        std::string name;
    };

    NodeCreateInfo getNodeCreateInfo(NodeType nodeType);

    class RenderGraph
    {
    public:
        nbl_DISABLE_COPY(RenderGraph);
        nbl_CTOR(RenderGraph);

        Node* addNode(const NodeCreateInfo& createInfo) noexcept;

        bool eraseNode(int32_t nodeId) noexcept;

        bool addEdge(int32_t startNodeId, int32_t startAttrId, int32_t endNodeId, int32_t endAttrId);

        bool addEdge(Node* startNode, const std::string& startAttrName, Node* endNode, const std::string& endAttrName);

        bool eraseEdge(int32_t edgeId);

        void reset();

        static int32_t nextId() noexcept;

        Node* getNode(int32_t id) noexcept;

        const std::vector<UPtr<Node>>& getNodes() const
        {
            return mNodes;
        }

        const std::vector<Edge>& getEdges() const
        {
            return mEdges;
        }

        const std::string& getName() const
        {
            return mName;
        }

        bool hasSourceNode() const
        {
            return mHasSourceNode;
        }

        bool hasSinkNode() const
        {
            return mHasSinkNode;
        }

        Node* getRootNode();

        void saveRenderGraph();

        void loadRenderGraph(const std::string& fileName);

    private:
        static int32_t sIdSequence;

        std::string                 mName;
        std::vector<UPtr<Node>>     mNodes;
        std::vector<Edge>           mEdges;

        bool                        mHasSourceNode = false;
        bool                        mHasSinkNode   = false;
    };
}
