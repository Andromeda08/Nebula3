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

    namespace detail
    {
        std::map<NodeType, NodeStyle>         getDefaultNodeStyles();
        std::map<ResourceType, ResourceStyle> getDefaultResourceStyles();
    }
}

struct RGConfiguration
{
    std::map<rg::NodeType, rg::NodeStyle>         nodeStyles            = rg::detail::getDefaultNodeStyles();
    std::map<rg::ResourceType, rg::ResourceStyle> resourceStyles        = rg::detail::getDefaultResourceStyles();
    ImGuiKey                                      bindDeleteEdge        = ImGuiKey_X;
    ImGuiKey                                      bindDeleteNode        = ImGuiKey_C;
    bool                                          alwaysGenInitialGraph = true;

    const rg::NodeStyle& getNodeStyle(rg::NodeType nodeType) const noexcept;

    const rg::ResourceStyle& getResourceStyle(rg::ResourceType resourceType) const noexcept;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RGConfiguration, nodeStyles, resourceStyles, bindDeleteEdge, bindDeleteNode, alwaysGenInitialGraph);
