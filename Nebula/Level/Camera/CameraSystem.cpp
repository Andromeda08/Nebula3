#include "CameraSystem.hpp"

#include <imgui.h>
#include "Core/Ranges.hpp"

namespace nbl
{
    CameraSystem::CameraSystem(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
        if (!mRHI)
        {
            exitWithError("RHI was null");
        }

        for (auto i = 0; i < mUniformBuffers.size(); i++)
        {
            mUniformBuffers[i] = mRHI->createBuffer({
                .size  = sizeof(GPUCameraData),
                .type  = RHI::BufferType::Uniform,
                .label = fmt::format("CameraSystem_UniformBuffer_{}", i),
            });
        }
    }

    void CameraSystem::onEvent(const SDL_Event& event) const noexcept
    {
        if (auto* camera = getActiveCamera())
        {
            camera->onEvent(event);
        }
    }

    void CameraSystem::onUpdate(const RHI::FrameData& frameData) const noexcept
    {
        if (auto* camera = getActiveCamera())
        {
            camera->onUpdate();

            const auto gpuCameraData = camera->getGpuCameraData();
            mUniformBuffers[frameData.currentFrame]->setData(&gpuCameraData, sizeof(GPUCameraData), 0);
        }
    }

    void CameraSystem::setActiveCamera(const int32_t index)
    {
        if (index < 0 || index >= mCameras.size())
        {
            spdlog::warn("Invalid camera index: {}", index);
            return;
        }

        mActiveCamera = index;
    }

    Camera* CameraSystem::getActiveCamera() const
    {
        return mActiveCamera == -1
            ? nullptr
            : mCameras[mActiveCamera].get();
    }

    const std::vector<UPtr<Camera>>& CameraSystem::getCameras() const
    {
        return mCameras;
    }

    int32_t CameraSystem::getActiveCameraIndex() const noexcept
    {
        return mActiveCamera;
    }

    const SPtr<RHI::Buffer>& CameraSystem::getBuffer(const uint32_t frameIndex) const noexcept
    {
        return mUniformBuffers[frameIndex];
    }

    CameraSystemUI::CameraSystemUI(CameraSystem* pCameraSystem)
    : IComponent()
    , mCameraSystem(pCameraSystem)
    {
        if (!mCameraSystem)
        {
            exitWithError("CameraSystem was null");
        }
    }

    void CameraSystemUI::draw()
    {
        ImGui::Begin("Camera System");

        const auto* activeCamera = mCameraSystem->getActiveCamera();
        const std::string activeName = activeCamera ? activeCamera->getName() : "None";

        ImGui::SeparatorText("Select Camera");
        if (ImGui::BeginCombo("##CameraSelect", activeName.c_str()))
        {
            for (const auto& [i, camera] : enumerate(mCameraSystem->getCameras()))
            {
                const std::string& name = camera->getName();
                const bool isSelected = i == mCameraSystem->getActiveCameraIndex();

                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    mCameraSystem->setActiveCamera(i);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::End();
    }
}
