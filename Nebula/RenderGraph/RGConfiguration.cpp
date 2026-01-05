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
        styles[HelloTrianglePresent] = {
            .cTitleBar        = {  98, 116, 143 },
            .cTitleBarSpecial = { 144, 161, 185 },
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
        styles[Image] = {
            .cPin  = { 124, 136, 255 },
            .cLink = {  97,  95, 255 },
        };
        styles[Image3D] = {
            .cPin  = {  55,  42, 172 },
            .cLink = {  55,  42, 172 },
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
