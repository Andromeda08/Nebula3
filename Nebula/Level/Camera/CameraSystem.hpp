#pragma once

// CameraSystem.hpp
// Manages multiple cameras and a per-frame uniform buffer.
// ============================================================

#include <vector>
#include "Camera.hpp"
#include "Core/Types.hpp"
#include "UserInterface/IComponent.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class CameraSystem
    {
    public:
        explicit CameraSystem(const SPtr<RHI::VulkanRHI>& rhi);

        void onEvent(const SDL_Event& event) const noexcept;

        void onUpdate(const RHI::FrameData& frameData) noexcept;

        template <class T, class... Args>
        requires std::derived_from<T, Camera>
        T* addCamera(const bool makeActive, Args&&... args)
        {
            mCameras.push_back(makeUnique<T>(std::forward<Args>(args)...));
            if (mActiveCamera == -1 || makeActive)
            {
                mActiveCamera = mCameras.size() - 1;
            }
            return static_cast<T*>(mCameras.back().get());
        }

        void setActiveCamera(int32_t index);

        [[nodiscard]] Camera* getActiveCamera() const;

        [[nodiscard]] const std::vector<UPtr<Camera>>& getCameras() const;

        [[nodiscard]] int32_t getActiveCameraIndex() const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getBuffer(uint32_t frameIndex) const noexcept;

        [[nodiscard]] const SPtr<RHI::Buffer>& getPreviousBuffer(uint32_t frameIndex) const noexcept;

    private:
        std::vector<UPtr<Camera>>           mCameras;
        int32_t                             mActiveCamera = -1;

        SPtr<RHI::VulkanRHI>                mRHI;
        PerFrameArray<SPtr<RHI::Buffer>>    mUniformBuffers;

        // Previous frame data, always 1 frame behind at the same index.
        GPUCameraData                       mPreviousData;
        PerFrameArray<SPtr<RHI::Buffer>>    mPreviousUniformBuffers;
    };

    class CameraSystemUI : public IComponent
    {
    public:
        explicit CameraSystemUI(CameraSystem* pCameraSystem);

        void draw() override;

    private:
        CameraSystem* mCameraSystem;
    };
}
