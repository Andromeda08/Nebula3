#include "PrePass.hpp"

#include "Templates.hpp"
#include "Level/Level.hpp"
#include "Level/TextureManager.hpp"

namespace nbl
{
    PrePass::PrePass(const PrePass_Params& params)
    : mRHI(params.rhi)
    , mLevel(params.pLevel)
    {
        using enum vk::ShaderStageFlagBits;

        for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mDepthBuffer[i]       = makeRenderTarget(mRHI.get(), fmt::format("PrePass_Depth_{}", i), vk::Format::eD32Sfloat);
            mObjInstanceBuffer[i] = makeRenderTarget(mRHI.get(), fmt::format("PrePass_ID_{}", i), vk::Format::eR32G32Sint);
        }

        const auto graphicsPS = RHI::GraphicsPS()
            // .addDefaultAttachmentState(1)
            .addAttachmentFormat(mDepthBuffer[0]->getProperties().format)
            // .addAttachmentFormat(mObjInstanceBuffer[0]->getProperties().format)
            .addVertexType<Vertex>();
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("PrePass")
            // .addDescriptorLayout(0, mLevel->mTextureManager->getDescriptor().get())
            .addShader("PrePass.vert.spv")
            // .addShader("PrePass.frag.spv")
            .setPushConstant<PushConstants>(eVertex);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void PrePass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
    {
        pCommandList->beginLabel("PrePass");

        RHI::Barrier()
            .addBarrier(mLevel->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(mLevel->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(mLevel->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("PrePass_RenderPass")
            // .addAttachment(mObjInstanceBuffer[frameData.currentFrame], vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::ClearValue().setColor({ -1, -1, 0, 0 }))
            .addAttachment(mDepthBuffer[frameData.currentFrame])
            .setRenderArea(mDepthBuffer[frameData.currentFrame]->getProperties().extent)
            .setViewportScissor(pCommandList)
            .insertBarriers(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd)
            {
                const auto [w, h] = mDepthBuffer[frameData.currentFrame]->getProperties().extent;
                const PushConstants pc = {
                    .instanceBuffer            = mLevel->mInstanceSystem->getBuffer()->getAddress(),
                    .instanceIndirectionBuffer = mLevel->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getAddress(),
                    .cameraBuffer              = mLevel->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                    .materialBuffer            = mLevel->mMaterialSystem->getBuffer()->getAddress(),
                };

                cmd->bindPipeline(mPipeline.get());
                // cmd->bindDescriptorSet(mLevel->mTextureManager->getDescriptor()->getSet(), 0);
                cmd->pushConstants(&pc);

                static constexpr vk::DeviceSize offsets[1] = { 0 };
                const auto& buffers = mLevel->mGeometrySystem->getBuffers();

                const std::array vertexBuffers { buffers.getVertexBuffer()->getHandle() };
                cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
                cmd->getHandle().bindIndexBuffer(buffers.getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

                mLevel->drawIndexedIndirect(cmd, frameData);
            });

        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& PrePass::getDepthBuffer(const uint32_t currentFrame) const
    {
        return mDepthBuffer[currentFrame];
    }

    const SPtr<RHI::Image>& PrePass::getObjInstanceBuffer(const uint32_t currentFrame) const
    {
        return mObjInstanceBuffer[currentFrame];
    }
}
