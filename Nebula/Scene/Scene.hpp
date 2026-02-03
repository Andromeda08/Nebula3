#pragma once

#include "TextureManager.hpp"
#include "Camera/ICamera.hpp"
#include "Core/Macro.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
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

    template <class T, class... Args>
    requires std::is_base_of_v<Geometry, T>
    T* addGeometry(Args&&... args) noexcept
    {
        mGeometries.push_back(makeUnique<T>(std::forward<Args>(args)...));
        return dynamic_cast<T*>(mGeometries.back().get());
    }

    std::vector<UPtr<Geometry>>         mGeometries;

    SPtr<RHI::Descriptor>               mSceneDescriptor;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUniformBuffers;

    UPtr<TextureManager>                mTextureManager;

    SPtr<RHI::VulkanRHI>                mRHI;

private:
    friend class SceneInfoComponent;

    std::vector<UPtr<ICamera>>          mCameras;
    ICamera*                            mActiveCamera;
    std::string                         mName;
};
