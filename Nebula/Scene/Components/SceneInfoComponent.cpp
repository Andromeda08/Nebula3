#include "SceneInfoComponent.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Random.hpp"
#include "Scene/Scene.hpp"

SceneInfoComponent::SceneInfoComponent(Scene* pScene)
: mScene(pScene)
{
}

void SceneInfoComponent::draw()
{
    ImGui::Begin("Scene Info");
    ImGui::Text("Name: %s", mScene->mName.c_str());

    // ============================
    // Lights
    // ============================
    auto* lights = mScene->mLights.get();
    ImGui::Text("Lights (%u)", static_cast<uint32_t>(lights->getCount()));
    ImGui::SameLine();
    if (ImGui::SmallButton("Add"))
    {
        mLightIndex = lights->addLight({
            .position   = glm::vec3(Random::get(-25.0f, 25.0f), Random::get(2.0f, 10.0f), Random::get(-25.0f, 25.0f)),
            .color      = Random::getVector<glm::vec3>(),
            .intensity  = 10000.0f,
            .enabled    = true,
            .type       = LightType::Point,
            .name       = "Light",
        });
    }
    ImGui::Separator();
    if (lights->getCount() == 0)
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No lights");
    }
    else
    {
        if (ImGui::BeginMenu("Select Light"))
        {
            for (const auto& idx : lights->getValidIndices())
            {
                if (ImGui::MenuItem(lights->mLights[idx].name.c_str(), nullptr, mLightIndex == idx, mLightIndex != idx))
                {
                    mLightIndex = idx;
                }
            }
            ImGui::EndMenu();
        }

        auto& light = lights->mLights[mLightIndex];
        const auto posLabel = toString(light.type);

        ImGui::InputText("Name", &light.name);

        bool changed = false;
        changed |= ImGui::InputFloat3(posLabel.data(), glm::value_ptr(light.position));
        changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
        changed |= ImGui::InputFloat("Intensity", &light.intensity);
        changed |= ImGui::Checkbox("Enabled", &light.enabled);

        if (changed)
        {
            lights->queueUpdate(mLightIndex);
        }
    }

    ImGui::End();
}