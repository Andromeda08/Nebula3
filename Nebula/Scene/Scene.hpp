#pragma once

#include <GLFW/glfw3.h>
#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "TextureManager.hpp"
#include "Camera/ICamera.hpp"
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

    void update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt)
    {
    }

private:
    friend class SceneInfoComponent;

    UPtr<ICamera>           mCamera;
    std::string             mName;
    UPtr<TextureManager>    mTextureManager;
};
