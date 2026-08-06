#include "Hybrid_MeshStage.hpp"

#include "Hair/HairGeometry.hpp"
#include "Level/Transform.hpp"

namespace nbl
{
    Hybrid_MeshStage::Hybrid_MeshStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared)
    : mRHI(rhi)
    , mShared(pShared)
    {
        using enum vk::ShaderStageFlagBits;
        const auto graphicsPS = RHI::GraphicsPS()
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(HairShared::sColorFormat)
            .addAttachmentFormat(HairShared::sDepthFormat)
            .setCullMode(vk::CullModeFlagBits::eNone);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("HybridHair_MeshStage_Pipeline")
            .addShader("HybridHair.task.spv")
            .addShader("HybridHair.mesh.spv")
            .addShader("HybridHair.frag.spv")
            .setPushConstant<PushConstants>(eMeshEXT | eTaskEXT | eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void Hybrid_MeshStage::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers)
    {
        pCommandList->beginLabel(mLabel);

        const auto& counterBuffer = mShared->smallTriangleCounterBuffer[frameData.currentFrame];

        // Clear counter buffer
        {
            pCommandList->beginLabel(mLabelPreamble);
            RHI::Barrier()
                .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
                .insert(pCommandList);
            pCommandList->getHandle().fillBuffer(counterBuffer->getHandle(), 0, sizeof(uint32_t), 0);
            pCommandList->endLabel();
        }

        // Dispatch mesh pipeline
        {
            const auto& trianglesBuffer = mShared->trianglesBuffer[frameData.currentFrame];

            pCommandList->beginLabel(mLabelMesh);
            RHI::Barrier()
                .addBarrier(mShared->colorTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
                .addBarrier(mShared->depthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
                .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::Compute_Write))
                .addBarrier(trianglesBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                .insert(pCommandList);

            using enum vk::AttachmentLoadOp;
            RHI::Rendering()
                .setRenderArea(mShared->renderResolution)
                .setViewportScissor(pCommandList)
                .addAttachment(mShared->colorTarget[frameData.currentFrame], mShared->config.renderHead ? eLoad : eClear)
                .addAttachment(mShared->depthBuffer[frameData.currentFrame], mShared->config.renderHead ? eLoad : eClear)
                .execute(pCommandList, [&](RHI::CommandList* cmd) -> void
                {
                    const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, 0.0f /* -45.0f */)).getModel();
                    const auto& info = mShared->hairModels->getHairInfo(mShared->config.hairIndex);

                    const auto pushConstants = PushConstants
                    {
                        .model                      = model,
                        .vertexBuffer               = mShared->hairModels->getVertexAddress(),
                        .attributesBuffer           = mShared->hairModels->getAttributesAddress(),
                        .strandDescriptionBuffer    = mShared->hairModels->getStrandDescriptionsAddress(),
                        .cameraBuffer               = buffers.cameraBuffer,
                        .lightsBuffer               = buffers.lightsBuffer,
                        .trianglesBuffer            = trianglesBuffer->getAddress(),
                        .firstVertex                = info.firstVertex,
                        .vertexCount                = info.vertexCount,
                        .firstStrand                = info.firstStrand,
                        .strandCount                = info.strandCount,
                        .smallTriangleCounterBuffer = counterBuffer->getAddress(),
                        .maxSmallTriangles          = info.vertexCount * 2,
                        .smallTriangleThreshold     = mShared->config.smallTriangleThreshold,
                        .viewport                   = mShared->viewportSize,
                        .isHybrid                   = mShared->config.isHybridMode ? 1 : 0,
                        .bsdf                       = mShared->config.bsdfParams,
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
}
