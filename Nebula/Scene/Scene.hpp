#pragma once

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "TextureManager.hpp"
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

    void update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt)
    {
    }

private:
    friend class SceneInfoComponent;

    std::string             mName;
    UPtr<TextureManager>    mTextureManager;
};
