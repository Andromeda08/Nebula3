#pragma once

#include <concepts>
#include <type_traits>
#include <vector>

#include "DependencyInfo.hpp"
#include "RenderGraphTraits.hpp"
#include "RGConfiguration.hpp"
#include "Core/Macro.hpp"

namespace rg
{
    struct NodeCreateInfo
    {
        NodeType                    nodeType;
        std::string                 displayName;
        std::string                 subTitle;
        std::vector<DependencyInfo> dependencies;
        NodeStyle                   nodeStyle;
    };

    template <class T>
    concept HasGetNodeInfoFunction = requires
    {
        { T::getNodeInfo() } -> std::same_as<NodeCreateInfo>;
    };

    class Node
    {
    public:
        static bool makeDirectedEdge(Node* src, Node* dst);
        static bool deleteDirectedEdge(Node* src, Node* dst);

        nbl_DISABLE_COPY(Node);
        nbl_CTOR(Node);

        void draw() const;

        DependencyInfo& getDependencyInfo(int32_t id);

        DependencyInfo& getDependencyInfo(const std::string& name);

        std::vector<DependencyInfo>& getDependencies()
        {
            return mDependencies;
        }

        const std::vector<DependencyInfo>& getDependencies() const
        {
            return mDependencies;
        }

        NodeType getNodeType() const
        {
            return mNodeType;
        }

        int32_t getId() const
        {
            return mId;
        }

        const std::string& getDisplayName() const
        {
            return mDisplayName;
        }

        const std::string& getSubTitle() const
        {
            return mSubTitle;
        }

        const std::vector<Node*>& getIncomingEdges() const
        {
            return mIncomingEdges;
        }

        int32_t getInDegree() const
        {
            return static_cast<int32_t>(mIncomingEdges.size());
        }

        const std::vector<Node*>& getOutgoingEdges() const
        {
            return mOutgoingEdges;
        }

        int32_t getOutDegree() const
        {
            return static_cast<int32_t>(mOutgoingEdges.size());
        }

        void setGridPos(const ImVec2& gridPos)
        {
            mGridPos = gridPos;
        }

        const ImVec2& getGridPos() const
        {
            return mGridPos;
        }

        [[nodiscard]] static UPtr<Node> createFromJson(const nlohmann::json& json, int32_t newId);

    private:
        Node(int32_t id, const std::string& name, const std::string& subTitle, NodeType nodeType, const NodeStyle& nodeStyle);

        const int32_t               mId = -1;
        const std::string           mDisplayName = "Unknown Node";
        const std::string           mSubTitle = "";
        std::vector<Node*>          mIncomingEdges;
        std::vector<Node*>          mOutgoingEdges;

        ImVec2                      mGridPos = {0, 0};

        std::vector<DependencyInfo> mDependencies;
        const NodeType              mNodeType = NodeType::Unknown;
        const NodeStyle             mStyle = {};
    };

    inline void to_json(nlohmann::json& json, const Node* node)
    {
        const auto incomingNodeIds = node->getIncomingEdges()
            | std::views::transform([](const Node* pNode) -> int32_t { return pNode->getId(); })
            | std::ranges::to<std::vector<int32_t>>();
        const auto outgoingNodeIds = node->getOutgoingEdges()
            | std::views::transform([](const Node* pNode) -> int32_t { return pNode->getId(); })
            | std::ranges::to<std::vector<int32_t>>();

        json = nlohmann::json {
            { "id", node->getId() },
            { "displayName", node->getDisplayName() },
            { "subTitle", node->getSubTitle() },
            { "gridPos", std::array{ node->getGridPos().x, node->getGridPos().y } },
            { "incomingNodeIds", incomingNodeIds },
            { "outgoingNodeIds", outgoingNodeIds },
            { "dependencies", node->getDependencies() },
            { "nodeType", node->getNodeType() }
        };
    }
}