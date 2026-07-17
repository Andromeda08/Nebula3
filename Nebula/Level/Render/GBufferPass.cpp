#include "GBufferPass.hpp"

#include "Templates.hpp"

namespace nbl
{
    GBufferPass::GBufferPass(const GBufferPass_Params& params)
    : mRHI(params.rhi)
    , mInput(params)
    {
        init();
    }

    void GBufferPass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const SPtr<RHI::Image>& prePassDepthBuffer) const noexcept
    {
        pCommandList->beginLabel("GBufferPass::execute()");

        const auto* level = mInput.pLevel;
        RHI::Barrier()
            .addBarrier(mWorldPosition->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mWorldNormal->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mMotionVectors->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mViewZ->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mAlbedoBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mEmissiveBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mParamsBuffer->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier((prePassDepthBuffer ? prePassDepthBuffer : mDepthBuffer)->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(level->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
            .insert(pCommandList);

        RHI::Rendering()
            .setLabel("G-Buffer_RenderPass")
            .setRenderArea(mWorldPosition->getProperties().extent)
            .addAttachment(mWorldPosition)
            .addAttachment(mWorldNormal)
            .addAttachment(mMotionVectors)
            .addAttachment(mViewZ)
            .addAttachment(mAlbedoBuffer)
            .addAttachment(mEmissiveBuffer)
            .addAttachment(mParamsBuffer)
            .addAttachment(prePassDepthBuffer ? prePassDepthBuffer : mDepthBuffer, prePassDepthBuffer ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void {
                const auto [w, h] = mWorldPosition->getProperties().extent;
                const PushConstants pc = {
                    .instanceBuffer            = level->mInstanceSystem->getBuffer()->getAddress(),
                    .instanceIndirectionBuffer = level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getAddress(),
                    .cameraBuffer              = level->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
                    .previousCameraBuffer      = level->mCameraSystem->getPreviousBuffer(frameData.currentFrame)->getAddress(),
                    .materialBuffer            = level->mMaterialSystem->getBuffer()->getAddress(),
                    .renderRes                 = glm::uvec2(w, h),
                };

                cmd->bindPipeline(mPipeline.get());
                cmd->bindDescriptorSet(mInput.pTextureManager->getDescriptor()->getSet(0), 0);
                cmd->pushConstants(&pc);

                static constexpr vk::DeviceSize offsets[1] = { 0 };
                const auto& buffers = level->mGeometrySystem->getBuffers();

                const std::array vertexBuffers { buffers.getVertexBuffer()->getHandle() };
                cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
                cmd->getHandle().bindIndexBuffer(buffers.getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);

                level->drawIndexedIndirect(cmd, frameData);
            });

        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& GBufferPass::getAlbedoBuffer() const noexcept
    {
        return mAlbedoBuffer;
    }

    const SPtr<RHI::Image>& GBufferPass::getDepthBuffer() const noexcept
    {
        return mDepthBuffer;
    }

    void GBufferPass::init() noexcept
    {
        constexpr auto samplerInfo = vk::SamplerCreateInfo()
            .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
            .setMinFilter(vk::Filter::eNearest)
            .setMagFilter(vk::Filter::eNearest);

        mWorldPosition  = makeRenderTarget(mRHI.get(), "GBuffer_WorldPosition", vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mWorldNormal    = makeRenderTarget(mRHI.get(), "GBuffer_WorldNormal",   vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mMotionVectors  = makeRenderTarget(mRHI.get(), "GBuffer_MV_DeltaViewZ", vk::Format::eR32G32B32A32Sfloat, std::nullopt, samplerInfo);
        mViewZ          = makeRenderTarget(mRHI.get(), "GBuffer_ViewZ",         vk::Format::eR32Sfloat,          std::nullopt, samplerInfo);
        mAlbedoBuffer   = makeRenderTarget(mRHI.get(), "GBuffer_Albedo_Clip",   vk::Format::eR16G16B16A16Sfloat, std::nullopt, samplerInfo);
        mEmissiveBuffer = makeRenderTarget(mRHI.get(), "GBuffer_Emissive",      vk::Format::eR8Uint,             std::nullopt, samplerInfo);
        mParamsBuffer   = makeRenderTarget(mRHI.get(), "GBuffer_Params",        vk::Format::eR16G16Sfloat,       std::nullopt, samplerInfo);
        mDepthBuffer    = makeRenderTarget(mRHI.get(), "GBuffer_Depth",         vk::Format::eD32Sfloat);

        const auto graphicsPS = RHI::GraphicsPS()
            .addDefaultAttachmentState(7)
            .addAttachmentFormat(mWorldPosition->getProperties().format)
            .addAttachmentFormat(mWorldNormal->getProperties().format)
            .addAttachmentFormat(mMotionVectors->getProperties().format)
            .addAttachmentFormat(mViewZ->getProperties().format)
            .addAttachmentFormat(mAlbedoBuffer->getProperties().format)
            .addAttachmentFormat(mEmissiveBuffer->getProperties().format)
            .addAttachmentFormat(mParamsBuffer->getProperties().format)
            .addAttachmentFormat(mDepthBuffer->getProperties().format)
            .configure([](auto& ps)
            {
                ps.depthStencilState
                    .setDepthWriteEnable(false)
                    .setDepthCompareOp(vk::CompareOp::eEqual);
            })
            .addVertexType<Vertex>();
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("G-Buffer")
            .addShader("GBuffer.vert.spv")
            .addShader("GBuffer.frag.spv")

            .addDescriptorLayout(0, mInput.pTextureManager->getDescriptor().get())
            .setPushConstant<PushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }
}
