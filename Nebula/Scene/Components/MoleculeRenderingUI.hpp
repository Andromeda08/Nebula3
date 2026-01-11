#pragma once

#include "Scene/Molecule/RenderingOptions.hpp"
#include "UserInterface/IComponent.hpp"

namespace Molecule
{
    class SDFComputePass;
    class StructurePass;
    class SDFRaymarchPass;
}

class MoleculeRenderingUI final : public IComponent
{
public:
    MoleculeRenderingUI(const MoleculeRenderingOptions* renderingOptions, Molecule::SDFComputePass* pComputePass, Molecule::StructurePass* pStructurePass, Molecule::SDFRaymarchPass* pRaymarchPass);

    ~MoleculeRenderingUI() override = default;

    void draw() override;

private:
    MoleculeRenderingOptions    mRenderingOptions;
    Molecule::SDFComputePass*   mComputePass;
    Molecule::StructurePass*    mStructurePass;
    Molecule::SDFRaymarchPass*  mSDFRaymarchPass;
    bool                        mUseSubsurfaceScattering = true;
};