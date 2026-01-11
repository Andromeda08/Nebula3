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
    ImGui::Text("Information");
    ImGui::Separator();
    ImGui::Text("Number of total Vertices: %u", static_cast<uint32_t>(mScene->mCIFData->getInfo().vertices));
    ImGui::Text("Number of atoms (spheres): %u", static_cast<uint32_t>(mScene->mCIFData->getInfo().atoms));
    ImGui::Text("Number of bonds (cylinders): %u", static_cast<uint32_t>(mScene->mCIFData->getInfo().bonds));
    ImGui::End();
}