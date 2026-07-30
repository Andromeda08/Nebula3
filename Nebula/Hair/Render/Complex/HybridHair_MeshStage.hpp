#pragma once

#include "Shared.hpp"
#include "Level/Transform.hpp"
#include "Level/Render/Templates.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    enum class VizMode : int32_t
    {
        HairColor           = 0,
        LongScattering      = 1,
        AzimuthalScattering = 2,
        CombinedScattering  = 3,
        Ambient             = 4,
        Diffuse             = 5,
        Marschner           = 6,
    };

    inline const char* toString(const VizMode e)
    {
        using enum VizMode;
        switch (e)
        {
            case HairColor:           return "HairColor (None)";
            case LongScattering:      return "LongScattering";
            case AzimuthalScattering: return "AzimuthalScattering";
            case CombinedScattering:  return "CombinedScattering";
            case Ambient:             return "Ambient";
            case Diffuse:             return "Diffuse";
            case Marschner:           return "Marschner";
            default:                  return "unknown";
        }
    }

    class HybridHair_MeshStage
    {
        struct PushConstants
        {
            glm::mat4 model;
            glm::vec4 diffuse;
            glm::vec4 specular;

            // Buffer References
            uint64_t  vertexBuffer;
            uint64_t  attributesBuffer;
            uint64_t  strandDescriptionBuffer;
            uint64_t  cameraBuffer;
            uint64_t  lights;
            uint64_t  trianglesBuffer;
            uint64_t  colorsBuffer;

            // Hair model specific global buffer offsets
            uint32_t  firstVertex;
            uint32_t  vertexCount;
            uint32_t  firstStrand;
            uint32_t  strandCount;

            // Path choosing
            uint64_t  smallTriangleCounterBuffer;
            uint32_t  maxSmallTriangles;
            float     smallTriangleThreshold;
            float     width;
            float     height;

            // Extras
            float     specularFactor;
            int32_t   useCustomColor;

            int32_t   isHybrid;

            // Marschner BSDF Params
            float     roughness         = glm::radians(6.0f);
            float     azimuthalWidth    = 0.3f;
            glm::vec3 absorption        = glm::vec3(0.4, 0.2, 0.05);
            float     shiftR            = glm::radians(-4.5f);
            float     shiftTT           = 0.0f;
            float     shiftTRT          = glm::radians(4.5f);
            float     scaleR            = 1.0f;
            float     scaleTT           = 1.0f;
            float     scaleTRT          = 0.5f;

            int32_t   vizMode           = 0;
        };

    public:
        HybridHair_MeshStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared)
        : mRHI(rhi)
        , mShared(pShared)
        {
            using enum VizMode;
            mVizModes = {
                toString(HairColor), toString(LongScattering), toString(AzimuthalScattering),
                toString(CombinedScattering), toString(Ambient), toString(Diffuse),
                toString(Marschner),
            };

            createResources();
            createPipeline();
        }

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers, const bool isHybridMode) const
        {
            pCommandList->beginLabel("MeshStage");

            const auto& counterBuffer = mShared->smallTriangleCounterBuffer[frameData.currentFrame];

            // Clear counter buffer
            {
                pCommandList->beginLabel("Preamble");
                RHI::Barrier()
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
                    .insert(pCommandList);
                pCommandList->getHandle().fillBuffer(counterBuffer->getHandle(), 0, sizeof(uint32_t), 0);
                pCommandList->endLabel();
            }

            // Dispatch mesh pipeline
            {
                const auto& trianglesBuffer = mShared->trianglesBuffer[frameData.currentFrame];

                pCommandList->beginLabel("Execution");
                RHI::Barrier()
                    .addBarrier(mRenderTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
                    .addBarrier(mDepthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::Compute_Write))
                    .addBarrier(trianglesBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                    .insert(pCommandList);

                RHI::Rendering()
                    .setRenderArea(mRenderTarget[0]->getProperties().extent)
                    .setViewportScissor(pCommandList)
                    .addAttachment(mRenderTarget[frameData.currentFrame])
                    .addAttachment(mDepthBuffer[frameData.currentFrame])
                    .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
                    {
                        const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, -45.0f)).getModel();
                        const auto& info = mShared->hairModels->getHairInfo(mShared->config.hairIndex);

                        const auto pushConstants = PushConstants
                        {
                            .model                      = model,
                            .diffuse                    = glm::vec4(mShared->config.diffuse, 1.0f),
                            .specular                   = glm::vec4(mShared->config.specular, 1.0f),
                            .vertexBuffer               = mShared->hairModels->getVertexAddress(),
                            .attributesBuffer           = mShared->hairModels->getAttributesAddress(),
                            .strandDescriptionBuffer    = mShared->hairModels->getStrandDescriptionsAddress(),
                            .cameraBuffer               = buffers.cameraBuffer,
                            .lights                     = buffers.lightsBuffer,
                            .trianglesBuffer            = trianglesBuffer->getAddress(),
                            .colorsBuffer               = mShared->colorsBuffer->getAddress(),
                            .firstVertex                = info.firstVertex,
                            .vertexCount                = info.vertexCount,
                            .firstStrand                = info.firstStrand,
                            .strandCount                = info.strandCount,
                            .smallTriangleCounterBuffer = counterBuffer->getAddress(),
                            .maxSmallTriangles          = info.vertexCount * 2,
                            .smallTriangleThreshold     = mShared->config.smallTriangleThreshold,
                            .width                      = static_cast<float>(mRenderTarget[0]->getProperties().extent.width),
                            .height                     = static_cast<float>(mRenderTarget[0]->getProperties().extent.height),
                            .specularFactor             = mShared->config.specularFactor,
                            .useCustomColor             = mShared->config.overrideColors ? 1 : 0,
                            .isHybrid                   = isHybridMode ? 1 : 0,
                            .roughness                  = glm::radians(6.0f),
                            .azimuthalWidth             = 0.3f,
                            .absorption                 = mAbsorption,
                            .shiftR                     = mShiftR,
                            .shiftTT                    = mShiftTT,
                            .shiftTRT                   = mShiftTRT,
                            .scaleR                     = mScaleR, // was 1.0f
                            .scaleTT                    = mScaleTT,
                            .scaleTRT                   = mScaleTRT,
                            .vizMode                    = std::to_underlying(mVizMode),
                        };

                        cmd->bindPipeline(mPipeline.get());
                        cmd->pushConstants(&pushConstants);

                        auto taskGroupSizeX = mShared->hairModels->getHairGeometry(static_cast<size_t>(mShared->config.hairIndex)).taskGroupSizeX;
                        if (mShared->config.useCustomWgSize)
                        {
                            taskGroupSizeX = mShared->config.customTaskWgSize;
                        }
                        cmd->drawMeshTasks(taskGroupSizeX, 1, 1);
                    });

                pCommandList->endLabel();
            }

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t frameIndex) const
        {
            return mRenderTarget[frameIndex];
        }

        void onDrawUI()
        {
            if (mAutoAbsorption)
            {
                mAbsorption = -glm::log(mShared->config.diffuse + glm::vec3(0.001));
            }

            ImGui::SeparatorText("Marschner BSDF");
            ImGui::Checkbox("Auto-Absorption", &mAutoAbsorption);

            ImGuiColorEditFlags flags = mAutoAbsorption ? ImGuiColorEditFlags_NoPicker : 0;
            ImGui::BeginDisabled(mAutoAbsorption);
            ImGui::ColorEdit3("Absorption", glm::value_ptr(mAbsorption), flags);
            ImGui::EndDisabled();

            ImGui::DragFloat("Scale R",   &mScaleR,   0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Scale TT",  &mScaleTT,  0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Scale TRT", &mScaleTRT, 0.01f, 0.0f, 1.0f);

            ImGui::SliderAngle("Shift R",   &mShiftR);
            ImGui::SliderAngle("Shift TT",  &mShiftTT);
            ImGui::SliderAngle("Shift TRT", &mShiftTRT);

            const auto currentMode = std::to_underlying(mVizMode);
            if (ImGui::BeginCombo("##Debug Visualization", mVizModes[currentMode].c_str()))
            {
                for (const auto& [i, camera] : enumerate(mVizModes))
                {
                    const bool isSelected = currentMode == i;

                    if (ImGui::Selectable(mVizModes[i].c_str(), isSelected))
                    {
                        mVizMode = static_cast<VizMode>(i);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

    private:
        void createResources()
        {
            for (size_t i = 0; i < mRenderTarget.size(); i++)
            {
                mRenderTarget[i] = makeRenderTarget(mRHI.get(), fmt::format("HybridHair_MeshStage_Target_{}", i));
                mDepthBuffer[i]  = makeRenderTarget(mRHI.get(), fmt::format("HybridHair_MeshStage_Depth_{}", i), vk::Format::eD32Sfloat);
            }
        }

        void createPipeline()
        {
            using enum vk::ShaderStageFlagBits;
            const auto graphicsPS = RHI::GraphicsPS()
                .addAlphaAttachmentState(1)
                .addAttachmentFormat(mRenderTarget[0]->getProperties().format)
                .addAttachmentFormat(mDepthBuffer[0]->getProperties().format)
                .setCullMode(vk::CullModeFlagBits::eNone);
            const auto pipelineInfo = RHI::PipelineCommon()
                .setLabel("HybridHair_MeshStage_Pipeline")
                .addShader("HybridHair.task.spv")
                .addShader("HybridHair.mesh.spv")
                .addShader("HybridHair.frag.spv")
                .setPushConstant<PushConstants>(eMeshEXT | eTaskEXT | eFragment);

            mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
        }

        bool      mAutoAbsorption = true;
        glm::vec3 mAbsorption     = glm::vec3(0.4f, 0.2f, 0.05f);

        float mScaleR   = 0.2f;
        float mShiftR   = glm::radians(-4.5f);

        float mScaleTT  = 1.0f;
        float mShiftTT  = 0.0f;

        float mScaleTRT = 0.5f;
        float mShiftTRT = glm::radians(4.5f);

        VizMode                                 mVizMode = VizMode::HairColor;
        std::vector<std::string>                mVizModes;

        SPtr<RHI::VulkanRHI>                    mRHI;
        HairShared*                             mShared;

        SPtr<RHI::GraphicsPipeline2>            mPipeline;
        PerFrameArray<SPtr<RHI::Image>>         mRenderTarget;
        PerFrameArray<SPtr<RHI::Image>>         mDepthBuffer;
    };
}
