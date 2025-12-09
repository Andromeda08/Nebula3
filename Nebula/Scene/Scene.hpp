#pragma once

#include "Core/Macro.hpp"
#include "Core/Types.hpp"

#include "TextureManager.hpp"

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

private:
    friend class SceneInfoComponent;

    std::string             mName;
    UPtr<TextureManager>    mTextureManager;
};
