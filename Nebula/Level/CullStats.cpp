#include "CullStats.hpp"

#include <imgui.h>

namespace nbl
{
    CullStatsUI::CullStatsUI(CullStats* pCullStats, bool* enableCulling)
    : mCullStats(pCullStats)
    , mEnableCulling(enableCulling)
    {
    }

    void CullStatsUI::draw()
    {
        ImGui::Begin("Culling");

        ImGui::SeparatorText("Options");
        ImGui::Checkbox("Enable Culling", mEnableCulling);

        ImGui::SeparatorText("Stats");
        ImGui::Text("Object Count: %d", mCullStats->totalObjectCount);
        ImGui::Text("Culled Count: %d (%.3f)", mCullStats->culledCount, mCullStats->percent);
        ImGui::Text("Time: %.3fms", mCullStats->cullTimeMs);

        ImGui::End();
    }
}
