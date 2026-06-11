#include "View.hpp"

#include <imgui.h>

namespace nbl
{
    void ViewSelectUI::draw()
    {
        ImGui::Begin("Views");
        {
            const auto hasActiveView = (*mActiveViewRef) != nullptr;

            const auto& current = hasActiveView ? (*mActiveViewRef)->getName() : "-";
            if (ImGui::BeginCombo("##ViewSelect", current.c_str()))
            {
                for (const auto& view: mViews | std::views::values)
                {
                    const bool isSelected = view->getName() == current;
                    if (ImGui::Selectable(view->getName().c_str(), isSelected))
                    {
                        (*mActiveViewRef) = view.get();
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }
        ImGui::End();
    }
}
