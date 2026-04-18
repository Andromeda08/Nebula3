#include "GBufferPass.hpp"

#include "Scene/SceneV2.hpp"

namespace nbl
{
    GBufferPass::GBufferPass(const GBufferPassCreateInfo& createInfo)
    : RenderPass(createInfo.params), mInput(createInfo.inputs)
    {
        mOutput = {
            .positionDepth = makeRenderTarget("PositionDepth"),
            .normal        = makeRenderTarget("Normal"),
            .albedo        = makeRenderTarget("Albedo"),
            .emissive      = makeRenderTarget("Emissive"),
            .depth         = makeRenderTarget("Depth", vk::Format::eD32Sfloat),
        };

        mRenderPass = mRHI->createRenderPass({
            .renderArea = mScissor,
            .colorAttachments = {
                makeColorAttachment(mOutput.positionDepth),
                makeColorAttachment(mOutput.normal),
                makeColorAttachment(mOutput.albedo),
                makeColorAttachment(mOutput.emissive),
            },
            .depthAttachment = makeDepthAttachment(mOutput.depth),
            .label = mName
        });

        const auto pipelineInfo = RHI::GraphicsPipelineCreateInfo()
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) -> void {
                    stateInfo
                        .addAttributeDescriptions<Vertex>(0, 0)
                        .addBindingDescriptions<Vertex>(0)
                        .addDefaultAttachmentStates(5);
                }))
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex)
            .addDescriptorSetLayout(mScene->getSceneDescriptor()->getLayout())
            .addDescriptorSetLayout(mScene->mTextureManager->getDescriptor()->getLayout())
            .addShader(RHI::ShaderStage::Vertex, Configuration::getSceneFilePath("IndirectDrawGBuffer.vert.spv"))
            .addShader(RHI::ShaderStage::Fragment, Configuration::getSceneFilePath("IndirectDrawGBuffer.frag.spv"))
            .addColorAttachmentFormat(mOutput.positionDepth->getProperties().format)
            .addColorAttachmentFormat(mOutput.normal->getProperties().format)
            .addColorAttachmentFormat(mOutput.albedo->getProperties().format)
            .addColorAttachmentFormat(mOutput.emissive->getProperties().format)
            .setDepthAttachmentFormat(mOutput.depth->getProperties().format)
            .setDebugName("GBufferPipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineInfo);
    }

    void GBufferPass::renderPass(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        setFullAreaViewportScissor(pCommandList);

        const auto& drawCommandsBuffer = mScene->mDrawCmdBuffer[frameData.currentFrame];
        const auto& instanceMapBuffer  = mScene->mInstanceMapBuffer[frameData.currentFrame];

        RHI::Barrier()
            .addBarrier(mOutput.positionDepth->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mOutput.normal->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mOutput.albedo->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mOutput.emissive->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mOutput.depth->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(instanceMapBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(drawCommandsBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect))
            .insert(pCommandList);

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            const PushConstants pc = {
                .instanceBufferAddress = mScene->mInstancePool->getBuffer()->getAddress(),
                .instanceMapAddress    = instanceMapBuffer->getAddress(),
            };

            mPipeline->bind(cmd);
            mPipeline->bindDescriptorSets(cmd, {
                mScene->getSceneDescriptor()->getSet(frameData.currentFrame),
                mScene->mTextureManager->getDescriptor()->getSet(0),
            });
            mPipeline->pushConstants(cmd, &pc);

            static constexpr vk::DeviceSize offsets[1] = { 0 };
            const auto [ vertexBuffer, indexBuffer, _ ] = mScene->mGeometry->getBuffers();

            const std::array vertexBuffers { vertexBuffer->getHandle() };
            cmd->getHandle().bindVertexBuffers(0, 1, vertexBuffers.data(), offsets);
            cmd->getHandle().bindIndexBuffer(indexBuffer->getHandle(), 0, vk::IndexType::eUint32);

            cmd->getHandle().drawIndexedIndirect(
                drawCommandsBuffer->getHandle(),
                0, mScene->mDrawCount, sizeof(vk::DrawIndexedIndirectCommand));
        });
    }
}
