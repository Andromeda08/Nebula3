#include "MoleculeSceneParamsComponent.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

using namespace Molecule;

MoleculeSceneParamsComponent::MoleculeSceneParamsComponent(MoleculeScene* pMoleculeScene)
: mScene(pMoleculeScene)
{
}

void MoleculeSceneParamsComponent::draw()
{
    auto* computePass      = mScene->mActiveMolecule->mSDFComputePass.get();
    auto* structurePass    = mScene->mActiveMolecule->mStructurePass.get();
    auto* raymarchPass     = mScene->mActiveMolecule->mSDFRaymarchPass.get();
    auto& renderingOptions = mScene->mMoleculeRenderingOptions;

    ImGui::Begin("Molecule Rendering Options");
    ImGui::Text("Current Molecule: %s", mScene->mActiveMolecule->mCIFData->getName().c_str());
    ImGui::Separator();

    if (ImGui::BeginMenu("Change Molecule"))
    {
        for (const auto& molecule : mScene->mLoadedMolecules)
        {
            if (ImGui::MenuItem(molecule.c_str(), nullptr, false, mScene->mActiveMolecule->mCIFData->getName() != molecule))
            {
                mScene->changeActiveMolecule(molecule);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    ImGui::ColorEdit4("Structure Color", glm::value_ptr(structurePass->mParams.structureColor));
    ImGui::ColorEdit4("Surface Color", glm::value_ptr(raymarchPass->mParams.sesColor));
    ImGui::SliderFloat("Blending amount", &raymarchPass->mParams.blending, 0.0f, 1.0f);
    ImGui::SliderFloat("Voxel size", &raymarchPass->mParams.voxelSize, 0.0f, 1.0f);
    ImGui::SliderInt("Ray Marching Steps", &raymarchPass->mParams.rayMarchingSteps, 0, 1024);

    ImGui::Text("Render Options");
    ImGui::Separator();
    ImGui::Checkbox("Render Structure", &renderingOptions.renderStructure);
    ImGui::Checkbox("Render Surface", &renderingOptions.renderSurface);

    ImGui::Text("SDF Options");
    ImGui::Separator();
    ImGui::Checkbox("Recalculate SDF (every frame, expensive)", &renderingOptions.shouldRecalculateSDF);
    ImGui::SliderFloat("Radius", &computePass->mPushConstants.radius, 0.0f, 5.0f);
    ImGui::SliderFloat("Scale", &computePass->mPushConstants.scale, 0.0f, 5.0f);

    ImGui::Text("Subsurface Scattering");
    ImGui::Separator();
    if (ImGui::Checkbox("Use Subsurface Scattering", &renderingOptions.useSubsurfaceScattering)) {
        raymarchPass->mParams.useSubsurfaceScattering = renderingOptions.useSubsurfaceScattering ? 1 : 0;
    }
    ImGui::SliderFloat("Depth", &raymarchPass->mParams.ls, 0.0f, 1.5f);
    ImGui::End();
}
