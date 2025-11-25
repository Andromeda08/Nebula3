#include "Node.hpp"

#include <imnodes.h>
#include "RenderGraph.hpp"

namespace rg
{
    bool Node::makeDirectedEdge(Node* src, Node* dst)
    {
        if (src->getId() == dst->getId())
        {
            return false;
        }

        src->mOutgoingEdges.push_back(dst);
        dst->mIncomingEdges.push_back(src);

        return true;
    }

    bool Node::deleteDirectedEdge(Node* src, Node* dst)
    {
        if (src->getId() == dst->getId())
        {
            return false;
        }

        const auto dstFind = std::ranges::find_if(src->mOutgoingEdges, [&dst](auto& v){ return v->getId() == dst->getId(); });
        if (dstFind == std::end(src->mOutgoingEdges))
        {
            return false;
        }

        const auto srcFind = std::ranges::find_if(dst->mIncomingEdges, [&src](auto& v){ return v->getId() == src->getId(); });
        if (srcFind == std::end(dst->mIncomingEdges))
        {
            return false;
        }

        src->mOutgoingEdges.erase(dstFind);
        dst->mIncomingEdges.erase(srcFind);

        return true;
    }

    Node::Node(const NodeCreateInfo& createInfo)
    : mId(RenderGraph::nextId())
    , mDisplayName(createInfo.displayName)
    , mDependencies(createInfo.dependencies)
    , mNodeType(createInfo.nodeType)
    , mStyle(createInfo.nodeStyle)
    {
    }

    void Node::draw() const
    {
        mStyle.pushColorStyles();
        ImNodes::BeginNode(mId);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(mDisplayName.c_str());
        ImNodes::EndNodeTitleBar();

        for (const auto& dependency : mDependencies)
        {
            if (dependency.dependencyType == DependencyType::Ignored)
            {
                continue;
            }

            const int32_t attributeId = dependency.id;
            ImNodes::PushColorStyle(ImNodesCol_Pin, nbl_TO_IM_COL32(dependency.style.cPin));

            switch (dependency.dependencyType)
            {
                case DependencyType::Expose:
                case DependencyType::Read: {
                    const ImNodesPinShape pinShape = dependency.isConnected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle;
                    ImNodes::BeginInputAttribute(attributeId, pinShape);
                    break;
                }
                case DependencyType::Write: {
                    ImNodes::BeginOutputAttribute(attributeId, ImNodesPinShape_CircleFilled);
                    break;
                }
                default: {
                    break;
                }
            }

            ImGui::Text("%s", dependency.name.c_str());

            switch (dependency.dependencyType)
            {
                case DependencyType::Ignored:
                case DependencyType::Read: {
                    ImNodes::EndInputAttribute();
                    break;
                }
                case DependencyType::Write: {
                    ImNodes::EndOutputAttribute();
                    break;
                }
                default: {
                    break;
                }
            }

            ImNodes::PopColorStyle(); // ImNodesCol_Pin
        }

        ImNodes::EndNode();
        mStyle.popColorStyles();
    }

    DependencyInfo& Node::getDependencyInfo(const int32_t id)
    {
        for (auto& dependency : mDependencies)
        {
            if (dependency.id == id)
            {
                return dependency;
            }
        }
        assert(false);
    }

    DependencyInfo& Node::getDependencyInfo(const std::string& name)
    {
        for (auto& dependency : mDependencies)
        {
            if (dependency.name == name)
            {
                return dependency;
            }
        }
        assert(false);
    }
}
