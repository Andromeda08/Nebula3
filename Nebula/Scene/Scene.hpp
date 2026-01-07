#pragma once

#include <GLFW/glfw3.h>

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "Camera/ICamera.hpp"
#include "Geometry/Geometry.hpp"
#include "Molecule/CIFData.hpp"
#include "RenderPass/Molecule/ComputePrePass.hpp"
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

    void handleInput(GLFWwindow* pWindow) const noexcept
    {
        mCamera->registerKeys(pWindow);
        mCamera->registerMouse(pWindow);
    }

    void update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt);

    void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData);

private:
    friend class SceneInfoComponent;

    UPtr<CIFData>                       mCIFData;

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<ICamera>                       mCamera;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUB;
    SPtr<RHI::Descriptor>               mSceneDescriptor;

    // Molecule: SDF
    UPtr<viz::ComputePrePass>           mComputePrePass;

    // Molecule: Structure Rendering
    glm::vec4                           mStructureColor = glm::vec4(0.45f, 0.2f, 0.8f, 1.0f);
    SPtr<RHI::Image>                    mDepthBuffer;
    SPtr<RHI::RenderPass>               mRenderPass;
    SPtr<RHI::GraphicsPipeline>         mStructurePipeline;

    SPtr<Geometry>                      mCube;
    SPtr<RHI::Buffer>                   mVertexBuffer;
    SPtr<RHI::Buffer>                   mIndexBuffer;
    SPtr<RHI::GraphicsPipeline>         mFwdPipeline;

    std::string                         mName;
};
