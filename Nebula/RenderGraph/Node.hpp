#pragma once

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
        std::vector<DependencyInfo> dependencies;
        NodeStyle                   nodeStyle;
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

    private:
        const int32_t               mId;
        const std::string           mDisplayName;
        std::vector<Node*>          mIncomingEdges;
        std::vector<Node*>          mOutgoingEdges;

        ImVec2                      mGridPos = {0, 0};

        std::vector<DependencyInfo> mDependencies;
        const NodeType              mNodeType = NodeType::Unknown;
        const NodeStyle&            mStyle;
    };
}