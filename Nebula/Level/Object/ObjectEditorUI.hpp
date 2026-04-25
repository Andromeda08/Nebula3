#pragma once

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "Object.hpp"
#include "UserInterface/IComponent.hpp"

namespace nbl
{
    class ObjectEditorUI : public IComponent
    {
    public:
        ObjectEditorUI(const std::vector<UPtr<Object>>& objects, int32_t* pSelectedObjectIdx)
        : mObjects(objects)
        , mSelectedObject(pSelectedObjectIdx)
        {
        }

        void draw() override
        {
            ImGui::Begin("Object Editor");

            if (*mSelectedObject == -1)
            {
                ImGui::Text("none");
            }
            else
            {
                auto* pSelectedObject = mObjects[*mSelectedObject].get();
                ImGui::Text("%s", pSelectedObject->name.c_str());
                if (ImGui::CollapsingHeader("Transform"))
                {
                    bool dirty = false;
                    dirty |= ImGui::DragFloat3("Position", glm::value_ptr(pSelectedObject->transform._translate), 1.0f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
                    dirty |= ImGui::DragFloat3("Scale", glm::value_ptr(pSelectedObject->transform._scale), 0.1f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
                    dirty |= ImGui::DragFloat3("Rotation", glm::value_ptr(pSelectedObject->transform._euler), 0.5f, -360.0f, 360.0f, "%.3f", ImGuiSliderFlags_WrapAround);
                    if (dirty)
                    {
                        pSelectedObject->isInstanceDirty = true;
                    }
                }
            }

            ImGui::End();
        }

    private:
        const std::vector<UPtr<Object>>& mObjects;
        int32_t*                         mSelectedObject;
    };
}
