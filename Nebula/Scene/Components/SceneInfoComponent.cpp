#include "SceneInfoComponent.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "Scene/Scene.hpp"

SceneInfoComponent::SceneInfoComponent(Scene* pScene)
: mScene(pScene)
, mUseSubsurfaceScattering(true)
, mRadius(mScene->mComputePrePass->getRadius())
, mScale(mScene->mComputePrePass->getScale())
{
}

void SceneInfoComponent::draw()
{
    ImGui::Begin("Scene Info");

    ImGui::Text("Name: %s", mScene->mName.c_str());
    ImGui::Separator();
    ImGui::ColorEdit4("Structure Color", glm::value_ptr(mScene->mStructureColor));
    ImGui::ColorEdit4("Surface Color", glm::value_ptr(mScene->mPCSDF.sesColor));
    ImGui::SliderFloat("Blending amount", &mScene->mPCSDF.blending, 0.0f, 1.0f);
    ImGui::SliderFloat("Voxel size", &mScene->mPCSDF.voxelSize, 0.0f, 1.0f);
    ImGui::SliderInt("Ray Marching Steps", &mScene->mPCSDF.rayMarchingSteps, 0, 1024);

    ImGui::Text("Render Options");
    ImGui::Separator();
    ImGui::Checkbox("Render Structure", &mScene->mSRO.renderStructure);
    ImGui::Checkbox("Render Surface", &mScene->mSRO.renderSurface);

    ImGui::Text("SDF Options");
    ImGui::Separator();
    ImGui::Checkbox("Recalculate SDF (every frame, expensive)", &mScene->mSRO.recalculateSDF);
    if (ImGui::SliderFloat("Radius", &mRadius, 0.0f, 5.0f)) {
        mScene->mComputePrePass->setRadius(mRadius);
    }
    if (ImGui::SliderFloat("Scale", &mScale, 0.0f, 5.0f)) {
        mScene->mComputePrePass->setScale(mScale);
    }

    ImGui::Text("Subsurface Scattering");
    ImGui::Separator();
    if (ImGui::Checkbox("Use Subsurface Scattering", &mUseSubsurfaceScattering)) {
        mScene->mPCSDF.useSubsurfaceScattering = mUseSubsurfaceScattering ? 1 : 0;
    }
    ImGui::SliderFloat("Depth", &mScene->mPCSDF.ls, 0.0f, 12.0f);

    ImGui::Text("Information");
    ImGui::Separator();
    ImGui::Text("Number of total Vertices: %u", mScene->mCIFData->getInfo().vertices);
    ImGui::Text("Number of atoms (spheres): %u", mScene->mCIFData->getInfo().atoms);
    ImGui::Text("Number of bonds (cylinders): %u", mScene->mCIFData->getInfo().bonds);
    ImGui::End();
}