#pragma once

#include <GLFW/glfw3.h>

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "Camera/ICamera.hpp"
#include "Geometry/Geometry.hpp"
#include "Molecule/CIFData.hpp"
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

    SPtr<RHI::Buffer> getMoleculePositionBuffer() const noexcept { return mMoleculePosBuffer; }

private:
    friend class SceneInfoComponent;

    UPtr<CIFData>                       mCIFData;
    SPtr<RHI::Buffer>                   mMoleculePosBuffer;

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<ICamera>                       mCamera;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUB;
    SPtr<RHI::Descriptor>               mSceneDescriptor;

    // Test scene with single cube
    SPtr<RHI::Image>                    mDepthBuffer;
    SPtr<RHI::RenderPass>               mRenderPass;
    SPtr<RHI::GraphicsPipeline>         mPipeline;
    SPtr<Geometry>                      mCube;
    SPtr<RHI::Buffer>                   mVertexBuffer;
    SPtr<RHI::Buffer>                   mIndexBuffer;

    std::string                         mName;
};
