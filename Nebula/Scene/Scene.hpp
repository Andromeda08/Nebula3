#pragma once

#include <GLFW/glfw3.h>

#include "TextureManager.hpp"
#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "Camera/ICamera.hpp"
#include "Molecule/CIFData.hpp"
#include "Molecule/RenderingOptions.hpp"
#include "RenderPass/Molecule/SDFComputePass.hpp"
#include "RenderPass/Molecule/SDFRaymarchPass.hpp"
#include "RenderPass/Molecule/StructurePass.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/Frame.hpp"

namespace RHI
{
    class Buffer;
    class CommandList;
    class Descriptor;
    class Image;
    class VulkanRHI;
}

struct SceneCreateInfo
{
    SPtr<RHI::VulkanRHI> rhi;
    std::string          name;
};

class Scene
{
public:
    nbl_DISABLE_COPY(Scene);
    nbl_CTOR(Scene);

    void registerUIComponents(UserInterface* pUserInterface) const;

    void onEvent(const SDL_Event& event) const noexcept
    {
        mCamera->onEvent(event);
    }

    void update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt);

    void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData);

private:
    friend class SceneInfoComponent;

    UPtr<CIFData>                       mCIFData;
    UPtr<TextureManager>                mTextureManager;

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<ICamera>                       mCamera;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUB;
    SPtr<RHI::Descriptor>               mSceneDescriptor;

    // Molecule Rendering
    MoleculeRenderingOptions            mMoleculeRenderingOptions;
    UPtr<Molecule::SDFComputePass>      mSDFComputePass;
    UPtr<Molecule::StructurePass>       mStructurePass;
    UPtr<Molecule::SDFRaymarchPass>     mSDFRaymarchPass;

    std::string                         mName;
};
