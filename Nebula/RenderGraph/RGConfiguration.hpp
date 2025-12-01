#pragma once

#include <array>
#include <map>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "RenderGraphTraits.hpp"

#define nbl_TO_IM_COL32(color) IM_COL32(color[0], color[1], color[2], 255)

namespace rg
{
    using Color = std::array<uint8_t, 3>;

    struct NodeStyle
    {
        Color cTitleBar        = { 128, 128, 128 };
        Color cTitleBarSpecial = { 196, 196, 196 };

        void pushColorStyles() const;
        void popColorStyles() const;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NodeStyle, cTitleBar, cTitleBarSpecial);

    struct ResourceStyle
    {
        Color cPin  = { 128, 128, 128 };
        Color cLink = { 196, 196, 196 };
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceStyle, cPin, cLink);

    std::map<NodeType, NodeStyle> getDefaultNodeStyles();

    std::map<ResourceType, ResourceStyle> getDefaultResourceStyles();
}

struct RGConfiguration
{
    std::map<rg::NodeType, rg::NodeStyle>         nodeStyles            = rg::getDefaultNodeStyles();
    std::map<rg::ResourceType, rg::ResourceStyle> resourceStyles        = rg::getDefaultResourceStyles();
    ImGuiKey                                      bindDeleteEdge        = ImGuiKey_X;
    ImGuiKey                                      bindDeleteNode        = ImGuiKey_C;
    bool                                          alwaysGenInitialGraph = true;

    const rg::NodeStyle& getNodeStyle(const rg::NodeType nodeType) noexcept
    {
        if (nodeStyles.contains(nodeType))
        {
            return nodeStyles[nodeType];
        }
        return nodeStyles[rg::NodeType::Unknown];
    }

    const rg::ResourceStyle& getResourceStyle(const rg::ResourceType resourceType) noexcept
    {
        if (resourceStyles.contains(resourceType))
        {
            return resourceStyles[resourceType];
        }
        return resourceStyles[rg::ResourceType::Unknown];
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RGConfiguration, nodeStyles, resourceStyles, bindDeleteEdge, bindDeleteNode, alwaysGenInitialGraph);