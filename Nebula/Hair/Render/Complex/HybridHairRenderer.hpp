#pragma once

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "HybridHair_DebugCompose.hpp"
#include "HybridHair_MeshStage.hpp"
#include "HybridHair_SoftwareStage.hpp"
#include "Shared.hpp"
#include "Hair/HairGeometry.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHairRenderer
    {
    public:
        HybridHairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModelSystem)
        : mRHI(rhi)
        , mHairModelSystem(pHairModelSystem)
        {
            mShared            = makeUnique<HairShared>(rhi.get(), mHairModelSystem);
            mMeshStage         = makeUnique<HybridHair_MeshStage>(mRHI, mShared.get());
            mSoftwareStage     = makeUnique<HybridHair_SoftwareStage>(mRHI, mShared.get());
            mDebugComposeStage = makeUnique<HybridHair_DebugCompose>(mRHI, mShared.get());
        }

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const uint64_t cameraBufferAddress) const
        {
            pCommandList->beginLabel("HybridHairRenderer");

            mMeshStage->execute(pCommandList, frameData, cameraBufferAddress);
            mSoftwareStage->execute(pCommandList, frameData);

            mDebugComposeStage->execute(pCommandList, frameData, mMeshStage->getResult(frameData.currentFrame), mSoftwareStage->getResult(frameData.currentFrame));

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t currentFrame) const noexcept
        {
            return mDebugComposeStage->getResult(currentFrame);
        }

    private:
        friend class HybridHairRendererUI;

        SPtr<RHI::VulkanRHI>            mRHI;
        HairModelSystem*                mHairModelSystem;

        UPtr<HairShared>                mShared;

        UPtr<HybridHair_MeshStage>      mMeshStage;
        UPtr<HybridHair_SoftwareStage>  mSoftwareStage;

        UPtr<HybridHair_DebugCompose>   mDebugComposeStage;
    };

    class HybridHairRendererUI : public IComponent
    {
    public:
        explicit HybridHairRendererUI(HybridHairRenderer* pHairRenderer)
        : IComponent()
        , mHairRenderer(pHairRenderer)
        , mShared(pHairRenderer->mShared.get())
        {
        }

        void draw() override
        {
            constexpr uint32_t uint32min = 0;

            ImGui::Begin("HairRenderer");

            const auto hairIndexMax = mHairRenderer->mHairModelSystem->getModelCount() - 1;
            ImGui::SliderScalar("Hair Model", ImGuiDataType_U32, &mShared->config.hairIndex, &uint32min, &hairIndexMax);

            ImGui::SeparatorText("Hybrid Config");
            ImGui::DragFloat("Alpha", &mShared->config.debugAlphaBlend, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat("Small Triangle Threshold", &mShared->config.smallTriangleThreshold, 0.1f, 0.0f, 8.0f);

            ImGui::SeparatorText("Task Workgroup Size");

            const auto defaultGroupSize = mHairRenderer->mHairModelSystem->getHairGeometry(mShared->config.hairIndex).taskGroupSizeX;

            ImGui::Text("Default size: %u", defaultGroupSize);
            ImGui::Checkbox("Override", &mShared->config.useCustomWgSize);
            ImGui::SliderScalar("X", ImGuiDataType_U32, &mShared->config.customTaskWgSize, &uint32min, &defaultGroupSize);

            ImGui::SeparatorText("Custom Color");
            ImGui::Checkbox("Enable", &mShared->config.overrideColors);
            ImGui::ColorEdit3("Diffuse", glm::value_ptr(mShared->config.diffuse));
            ImGui::ColorEdit3("Specular", glm::value_ptr(mShared->config.specular));
            ImGui::DragFloat("Specular", &mShared->config.specularFactor, 0.05f, 0.0f, 32.0f);

            ImGui::End();
        }

    private:
        HybridHairRenderer* mHairRenderer;
        HairShared*         mShared;
    };
}
