#include "RGConfiguration.hpp"

#include <imnodes.h>

namespace rg
{
    void NodeStyle::pushColorStyles() const
    {
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, nbl_TO_IM_COL32(cTitleBar));
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, nbl_TO_IM_COL32(cTitleBarSpecial));
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, nbl_TO_IM_COL32(cTitleBarSpecial));
    }

    void NodeStyle::popColorStyles() const
    {
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
    }

    namespace detail
    {
        std::map<NodeType, NodeStyle> getDefaultNodeStyles()
        {
            using enum NodeType;
            std::map<NodeType, NodeStyle> styles;

            styles[Unknown] = {};
            styles[Scene] = {
                .cTitleBar        = { 255,  32,  86 },
                .cTitleBarSpecial = { 255,  99, 126 },
            };
            styles[Present] = {
                .cTitleBar        = { 255,  32,  86 },
                .cTitleBarSpecial = { 255,  99, 126 },
            };
            styles[GBufferPass] = {
                .cTitleBar        = {   0, 184, 219 },
                .cTitleBarSpecial = {   0, 211, 242 },
            };
            styles[LightingPass] = {
                .cTitleBar        = {   0, 187, 167 },
                .cTitleBarSpecial = {   0, 213, 190 },
            };
            styles[AntiAliasingPass] = {
                .cTitleBar        = { 173,  70, 255 },
                .cTitleBarSpecial = { 104, 122, 255 },
            };
            styles[AmbientOcclusionPass] = {
                .cTitleBar        = { 251,  44,  54 },
                .cTitleBarSpecial = { 255, 100, 103 },
            };

            return styles;
        }

        std::map<ResourceType, ResourceStyle> getDefaultResourceStyles()
        {
            using enum ResourceType;
            std::map<ResourceType, ResourceStyle> styles;

            styles[Unknown] = {};
            styles[Buffer] = {
                .cPin  = { 255, 100, 103 },
                .cLink = { 251,  44,  54 },
            };
            styles[Texture2D] = {
                .cPin  = { 124, 136, 255 },
                .cLink = {  97,  95, 255 },
            };
            styles[Texture3D] = {
                .cPin  = {  67,  45, 215 },
                .cLink = {  67,  45, 215 },
            };
            styles[SceneData] = {
                .cPin  = { 255, 186,   0 },
                .cLink = { 253, 154,   0 },
            };
            styles[TopLevelAS] = {
                .cPin  = { 154, 230,   0 },
                .cLink = { 124, 207,   0 },
            };

            return styles;
        }
    }
}

const rg::NodeStyle& RGConfiguration::getNodeStyle(const rg::NodeType nodeType) const noexcept
{
    if (nodeStyles.contains(nodeType))
    {
        return nodeStyles.at(nodeType);
    }
    return nodeStyles.at(rg::NodeType::Unknown);
}

const rg::ResourceStyle& RGConfiguration::getResourceStyle(const rg::ResourceType resourceType) const noexcept
{
    if (resourceStyles.contains(resourceType))
    {
        return resourceStyles.at(resourceType);
    }
    return resourceStyles.at(rg::ResourceType::Unknown);
}
