#pragma once

#include "RenderPass/Molecule/SDFComputePass.hpp"
#include "RenderPass/Molecule/SDFRaymarchPass.hpp"
#include "RenderPass/Molecule/StructurePass.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Molecule/CIFData.hpp"
#include "Scene/Molecule/RenderingOptions.hpp"

class MoleculeScene : public Scene
{
public:
    explicit MoleculeScene(const SceneCreateInfo& createInfo);

    void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept override;

    static void registerUIComponent(MoleculeScene* pMoleculeScene, UserInterface* pUserInterface) noexcept;

private:
    void findAndLoadMolecules() noexcept;

    void changeActiveMolecule(const std::string& name) noexcept;

    friend class MoleculeSceneParamsComponent;

    struct MoleculeData
    {
        UPtr<CIFData>                       mCIFData;
        UPtr<Molecule::SDFComputePass>      mSDFComputePass;
        UPtr<Molecule::StructurePass>       mStructurePass;
        UPtr<Molecule::SDFRaymarchPass>     mSDFRaymarchPass;
    };

    // Molecule Data
    std::vector<MoleculeData>   mMolecules;
    std::vector<std::string>    mLoadedMolecules;
    MoleculeData*               mActiveMolecule;
    MoleculeRenderingOptions    mMoleculeRenderingOptions;

};
