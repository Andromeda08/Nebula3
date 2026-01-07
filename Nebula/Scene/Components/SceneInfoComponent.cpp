#include "SceneInfoComponent.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "Scene/Scene.hpp"

SceneInfoComponent::SceneInfoComponent(Scene* pScene)
: mScene(pScene)
{
}

void SceneInfoComponent::draw()
{
    ImGui::Begin("Scene Info");
    ImGui::Text("Name: %s", mScene->mName.c_str());
    ImGui::ColorEdit4("Structure Color", glm::value_ptr(mScene->mStructureColor));
    ImGui::End();
}