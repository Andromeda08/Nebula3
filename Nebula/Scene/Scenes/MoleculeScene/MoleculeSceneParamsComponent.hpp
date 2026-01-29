#pragma once

#include "MoleculeScene.hpp"
#include "UserInterface/IComponent.hpp"

namespace Molecule
{
    class SDFComputePass;
    class StructurePass;
    class SDFRaymarchPass;
}

class MoleculeSceneParamsComponent final : public IComponent
{
public:
    explicit MoleculeSceneParamsComponent(MoleculeScene* pMoleculeScene);

    ~MoleculeSceneParamsComponent() override = default;

    void draw() override;

private:
    MoleculeScene*  mScene;
    bool           mUseSubsurfaceScattering = true;
};