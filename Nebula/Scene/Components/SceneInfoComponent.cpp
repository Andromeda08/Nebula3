#include "SceneInfoComponent.hpp"

#include <imgui.h>
#include "Scene/Scene.hpp"

SceneInfoComponent::SceneInfoComponent(Scene* pScene)
: mScene(pScene)
{
}

void SceneInfoComponent::draw()
{
    ImGui::Begin("Scene Info");
    ImGui::Text("Name: %s", mScene->mName.c_str());
    ImGui::End();
}