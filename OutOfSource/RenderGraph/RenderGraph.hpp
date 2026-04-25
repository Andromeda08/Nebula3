#pragma once

#include <expected>
#include <set>
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
        DependencyInfo* pSrcDependency;
        Node*           pDst;
        int32_t         dstDependencyId;
        DependencyInfo* pDstDependency;
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

        Node* getRootNode() const;

        [[nodiscard]] std::expected<nlohmann::json, std::string> serializeRenderGraph() const;

        [[nodiscard]] static std::expected<UPtr<RenderGraph>, std::string> deserializeRenderGraph(const nlohmann::json& json, const std::set<NodeType>& supportedNodes);

        bool getSupportsCurrentPlatform() const
        {
            return mSupportsCurrentPlatform;
        }

        std::string* getPName()
        {
            return &mName;
        }

        void setName(const std::string& name) noexcept
        {
            mName = name;
        }

        bool isHidden() const noexcept
        {
            return mIsHidden;
        }

        void setHidden(const bool value) noexcept
        {
            mIsHidden = value;
        }

    private:
        static int32_t sIdSequence;

        std::string                 mName;
        std::vector<UPtr<Node>>     mNodes;
        std::vector<Edge>           mEdges;

        bool                        mHasSourceNode = false;
        bool                        mHasSinkNode   = false;

        bool                        mSupportsCurrentPlatform = true;
        bool                        mIsHidden                = false;
    };
}
