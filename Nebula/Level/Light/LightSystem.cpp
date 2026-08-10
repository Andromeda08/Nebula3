#include "LightSystem.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Random.hpp"
#include "Core/Ranges.hpp"

namespace nbl
{
    void LightSystem::onDrawUI()
    {
        ImGui::Begin("Light System");

        ImGui::Text("Count: (%u)", getSize());

        // Add Function
        // ============================
        ImGui::SameLine();
        if (ImGui::SmallButton("Add"))
        {
            const auto      type   = static_cast<LightType>(Random::get<int32_t>(0, 1));
            const glm::vec3 vector = (type == LightType::Point)
                ? glm::vec3(Random::get(-25.0f, 25.0f), Random::get(2.0f, 10.0f), Random::get(-25.0f, 25.0f))
                : Random::getUnitVector<glm::vec3>();

            mSelectedLight = acquire({
                .vector       = vector,
                .color        = Random::getVector<glm::vec3>(),
                .intensity    = Random::get(100.0f, 1000.0f),
                .isEnabled    = true,
                .castsShadows = true,
                .radius       = Random::get(5.0f, 25.0f),
                .type         = type,
                .name         = fmt::format("Light {}", getSize()),
            });
        }

        // Remove Function
        // ============================
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            release(mSelectedLight);
            mSelectedLight = (getSize() > 0) ? getHandleFromDense(0) : PoolHandle {};
        }

        // Select Function
        // ============================
        ImGui::Separator();
        if (getSize() == 0)
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No lights");
        }
        else
        {
            if (ImGui::BeginMenu("Select Light"))
            {
                forEachView([&](CpuView<Light> lightView) -> void {
                    const auto& [ handle, light ] = lightView;
                    const bool isSelected = handle == mSelectedLight;
                    const bool isEnabled = !isSelected;

                    if (ImGui::MenuItem(light->name.c_str(), nullptr, isSelected, isEnabled))
                    {
                        mSelectedLight = handle;
                    }
                });
                ImGui::EndMenu();
            }
        }

        // Edit Selected Function
        // ============================
        if (!mSelectedLight.isNull())
        {
            auto        light    = Light(*get(mSelectedLight));
            bool        changed  = false;

            ImGui::SeparatorText("Edit Light");

            const auto currentType = std::to_underlying(light.type);
            if (ImGui::BeginCombo("##LightType", mLightTypeNames[currentType].c_str()))
            {
                for (const auto& [i, camera] : enumerate(mLightTypeNames))
                {
                    const bool isSelected = currentType == i;

                    if (ImGui::Selectable(mLightTypeNames[i].c_str(), isSelected))
                    {
                        light.type = static_cast<LightType>(i);
                        changed = true;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            changed |= ImGui::InputText("Name", &light.name);
            changed |= ImGui::DragFloat3(mLightTypeNames[currentType].c_str(), glm::value_ptr(light.vector), 0.1f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
            changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
            changed |= ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, std::numeric_limits<float>::max());
            changed |= ImGui::Checkbox("Enabled", &light.isEnabled);
            changed |= ImGui::Checkbox("Shadows", &light.castsShadows);

            if (changed)
            {
                update(mSelectedLight, light);
            }
        }

        ImGui::End();
    }
}
