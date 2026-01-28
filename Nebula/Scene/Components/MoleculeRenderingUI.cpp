#include "MoleculeRenderingUI.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "RenderPass/Molecule/SDFComputePass.hpp"
#include "RenderPass/Molecule/SDFRaymarchPass.hpp"
#include "RenderPass/Molecule/StructurePass.hpp"

using namespace Molecule;

MoleculeRenderingUI::MoleculeRenderingUI(const MoleculeRenderingOptions* renderingOptions, SDFComputePass* pComputePass, StructurePass* pStructurePass, SDFRaymarchPass* pRaymarchPass)
: mRenderingOptions(renderingOptions)
, mComputePass(pComputePass)
, mStructurePass(pStructurePass)
, mSDFRaymarchPass(pRaymarchPass)
{
}

void MoleculeRenderingUI::draw()
{
    ImGui::Begin("Molecule Rendering Options");
    ImGui::ColorEdit4("Structure Color", glm::value_ptr(mStructurePass->mParams.structureColor));
    ImGui::ColorEdit4("Surface Color", glm::value_ptr(mSDFRaymarchPass->mParams.sesColor));
    ImGui::SliderFloat("Blending amount", &mSDFRaymarchPass->mParams.blending, 0.0f, 1.0f);
    ImGui::SliderFloat("Voxel size", &mSDFRaymarchPass->mParams.voxelSize, 0.0f, 1.0f);
    ImGui::SliderInt("Ray Marching Steps", &mSDFRaymarchPass->mParams.rayMarchingSteps, 0, 1024);

    ImGui::Text("Render Options");
    ImGui::Separator();
    ImGui::Checkbox("Render Structure", &mRenderingOptions.renderStructure);
    ImGui::Checkbox("Render Surface", &mRenderingOptions.renderSurface);

    ImGui::Text("SDF Options");
    ImGui::Separator();
    ImGui::Checkbox("Recalculate SDF (every frame, expensive)", &mRenderingOptions.shouldRecalculateSDF);
    ImGui::SliderFloat("Radius", &mComputePass->mPushConstants.radius, 0.0f, 5.0f);
    ImGui::SliderFloat("Scale", &mComputePass->mPushConstants.scale, 0.0f, 5.0f);

    ImGui::Text("Subsurface Scattering");
    ImGui::Separator();
    if (ImGui::Checkbox("Use Subsurface Scattering", &mUseSubsurfaceScattering)) {
        mSDFRaymarchPass->mParams.useSubsurfaceScattering = mUseSubsurfaceScattering ? 1 : 0;
    }
    ImGui::SliderFloat("Depth", &mSDFRaymarchPass->mParams.ls, 0.0f, 1.5f);
    ImGui::End();
}
