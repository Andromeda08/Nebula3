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

}
