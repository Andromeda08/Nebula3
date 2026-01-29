#pragma once

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

    virtual ~Scene() = default;

    virtual void registerUIComponents(UserInterface* pUserInterface) const noexcept;

    virtual void onEvent(const SDL_Event& event) const noexcept;

    virtual void onUpdate(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, float dt) noexcept;

    virtual void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept;

protected:
    /**
     * Add a new Camera to the scene.
     * @param camera UPtr Camera
     * @param makeActive Also set the new camera as the active one. (always true if first camera)
     */
    void addCamera(UPtr<ICamera> camera, bool makeActive = false) noexcept;

    SPtr<RHI::VulkanRHI>                mRHI;
    SPtr<RHI::Descriptor>               mSceneDescriptor;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUniformBuffers;

private:
    // Interface version
    std::vector<UPtr<ICamera>>          mCameras;
    ICamera*                            mActiveCamera;

    friend class SceneInfoComponent;

    UPtr<CIFData>                       mCIFData;
    UPtr<TextureManager>                mTextureManager;

    UPtr<ICamera>                       mCamera;

    // Molecule Rendering
    MoleculeRenderingOptions            mMoleculeRenderingOptions;
    UPtr<Molecule::SDFComputePass>      mSDFComputePass;
    UPtr<Molecule::StructurePass>       mStructurePass;
    UPtr<Molecule::SDFRaymarchPass>     mSDFRaymarchPass;

    std::string                         mName;
};
