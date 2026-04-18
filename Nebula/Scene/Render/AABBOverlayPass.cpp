#include "AABBOverlayPass.hpp"

#include "Scene/SceneV2.hpp"

AABBOverlayPass::AABBOverlayPass(const AABBOverlay_Params& params)
: RenderPass({ params.resolution, params.rhi, "AABB_Overlay" })
, mInput(params.input)
{
    createPipeline();
}

UPtr<AABBOverlayPass> AABBOverlayPass::create(const AABBOverlay_Params& params) noexcept
{
    return makeUnique<AABBOverlayPass>(params);
}

void AABBOverlayPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    pCommandList->beginLabel("Lighting_Pass");

    setScissorViewport(pCommandList);

    RHI::Barrier()
        .addBarrier(mInput.image->getBarrier(RHI::ImageUsage::ColorAttachment))
        .addBarrier(mInput.depthBuffer->getBarrier(RHI::ImageUsage::DepthAttachment))
        .insert(pCommandList);

    mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        mPipeline->bind(cmd);
        mPipeline->bindDescriptorSet(cmd, mInput.pScene->getSceneDescriptor()->getSet(frameData.currentFrame));
        for (auto i = 0; i < mInput.pScene->getObjects().size(); ++i)
        {
            PushConstants pushConstants = {
                .min = glm::vec4(mInput.pScene->getObjects()[i]->boundingBox.getMin(), 1.0f),
                .max = glm::vec4(mInput.pScene->getObjects()[i]->boundingBox.getMax(), 1.0f),
            };
            mPipeline->pushConstants(cmd, &pushConstants);
            cmd->getHandle().draw(24, 1, 0, 0);
        }
    });
    pCommandList->endLabel();
}

void AABBOverlayPass::createPipeline() noexcept
{
    mRenderPass = mRHI->createRenderPass({
        .renderArea = getRenderArea(),
        .colorAttachments = {
            RHI::Attachment {
                .image = mInput.image->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mInput.image->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                },
            },
        .depthAttachment  = RHI::Attachment {
            .image = mInput.depthBuffer->getImage(),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setImageView(mInput.depthBuffer->getImageView())
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eNone),
        },
        .label = "AABB_Overlay_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants) })
        .addDescriptorSetLayout(mInput.pScene->getSceneDescriptor()->getLayout())
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentStates(1)
            .configure([](auto& state) {
                state.inputAssemblyState.setTopology(vk::PrimitiveTopology::eLineList);
            }))
        .addShader({ Configuration::getShaderFilePath("AABB.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
        .addShader({ Configuration::getShaderFilePath("AABB.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mInput.image->getProperties().format)
        .setDepthAttachmentFormat(mInput.depthBuffer->getProperties().format)
        .setDebugName("AABB_Overlay_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}
