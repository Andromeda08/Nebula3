#include "SceneInfoComponent.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Random.hpp"
#include "Scene/SceneV2.hpp"

SceneInfoComponent::SceneInfoComponent(SceneV2* pScene)
: mScene(pScene)
{
}

void SceneInfoComponent::draw()
{
    ImGui::Begin("Scene Info");
    ImGui::Text("Name: %s", mScene->mName.c_str());

    // Lights
    // ============================
    auto* lights = mScene->mLightSystem.get();
    ImGui::Text("Lights (%u)", static_cast<uint32_t>(lights->getCount()));
    ImGui::SameLine();
    if (ImGui::SmallButton("Add"))
    {
        mLightIndex = lights->addLight({
            .vector      = glm::vec3(Random::get(-25.0f, 25.0f), Random::get(2.0f, 10.0f), Random::get(-25.0f, 25.0f)),
            .color       = Random::getVector<glm::vec3>(),
            .intensity   = 1500.0f,
            .isEnabled   = true,
            .castsShadow = true,
            .radius      = 10.0f,
            .type        = LightType::Point,
            .name        = "Light",
        });
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove"))
    {
        lights->removeLight(mLightIndex);
        lights->queueUpdate(mLightIndex);

        const auto& i = lights->getValidIndices();
        mLightIndex = *std::ranges::min_element(i);
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
        changed |= ImGui::DragFloat3(posLabel.data(), glm::value_ptr(light.vector), 0.1f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
        changed |= ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, std::numeric_limits<float>::max());
        changed |= ImGui::Checkbox("Enabled", &light.isEnabled);
        changed |= ImGui::Checkbox("Shadows", &light.castsShadow);

        if (changed)
        {
            lights->queueUpdate(mLightIndex);
        }
    }

    // Shadows
    // ============================
    ImGui::SeparatorText("Shadows");
    if (ImGui::SmallButton("Disable"))
    {
        mScene->mLightingPass->setShadowMode(0);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Enable"))
    {
        mScene->mLightingPass->setShadowMode(1);
    }

    // Culling
    // ============================
    ImGui::SeparatorText("Culling");
    ImGui::Checkbox("Enable Culling", &mScene->mEnableCulling);
    ImGui::Checkbox("Visualize AABBs", &mScene->mVisualizeAABBs);

    ImGui::SeparatorText("Culling Stats");
    ImGui::Text("Object Count: %d", mScene->mLastCull.totalObjectCount);
    ImGui::Text("Culled Count: %d (%.3f)", mScene->mLastCull.culledCount, mScene->mLastCull.percent);
    ImGui::Text("Time: %.3fms", mScene->mLastCull.cullTimeMs);

    ImGui::End();
}