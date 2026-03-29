#pragma once

#include "RenderPass.hpp"
#include "Scene/LightSystem.hpp"
#include "Scene/SceneGeometry.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

struct LightDebugOverlay_Params
{
    Size2D                resolution;
    SPtr<RHI::Descriptor> sceneDescriptor;
    SPtr<RHI::Image>      depthBuffer;
    SPtr<RHI::Image>      input;
    SceneGeometry*        pGeometry;
    SPtr<RHI::VulkanRHI>  rhi;
};

class LightDebugOverlay : public RenderPass
{
    struct PushConstant
    {
        float alpha;
    };
public:
    explicit LightDebugOverlay(const LightDebugOverlay_Params& params)
    : RenderPass({ params.resolution, params.rhi, "LightDebugOverlay" })
    , mInput(params.input)
    , mSceneDescriptor(params.sceneDescriptor)
    , mSceneGeometry(params.pGeometry)
    , mDepthBuffer(params.depthBuffer)
    {
        createPipeline();
        mPushConstants = {
            .alpha = 0.75f,
        };
    }

    [[nodiscard]] static UPtr<LightDebugOverlay> create(const LightDebugOverlay_Params& params) noexcept
    {
        return makeUnique<LightDebugOverlay>(params);
    }

    void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept override
    {
        pCommandList->beginLabel("LightDebugOverlay_Pass");
        RHI::Barrier()
            .addBarrier(mInput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        const auto& sphereInfo = mSceneGeometry->getGeometryInfo(Sphere::sName);

        mRenderPass->execute(pCommandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
            mPipeline->bind(commandBuffer);
            mPipeline->pushConstants(commandBuffer, &mPushConstants);
            mPipeline->bindDescriptorSet(commandBuffer, mSceneDescriptor->getSet(frameData.currentFrame));

            constexpr vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &mSceneGeometry->getVertexBuffer()->getHandle(), &offset);
            commandBuffer.bindIndexBuffer(mSceneGeometry->getIndexBuffer()->getHandle(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(sphereInfo.indexRegion.indexCount, LightSystem::sMaxLights, sphereInfo.indexRegion.firstIndex, sphereInfo.vertexRegion.firstVertex, 0);
        });

        pCommandList->endLabel();
    }

    [[nodiscard]] const SPtr<RHI::Image>& getResult() const noexcept
    {
        return mInput;
    }

private:
    void createPipeline() noexcept
    {
        mRenderPass = mRHI->createRenderPass({
            .renderArea = getRenderArea(),
            .colorAttachments = {
                RHI::Attachment {
                    .image = mInput->getImage(),
                    .attachmentInfo = vk::RenderingAttachmentInfo()
                        .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
                        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                        .setImageView(mInput->getImageView())
                        .setLoadOp(vk::AttachmentLoadOp::eLoad)
                        .setStoreOp(vk::AttachmentStoreOp::eStore)
                },
            },
            .depthAttachment = RHI::Attachment {
                .image = mDepthBuffer->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                    .setImageView(mDepthBuffer->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eNone)
            },
            .label = "LightDebugOverlay_RenderPass",
        });

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant) })
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .addAttachmentState(RHI::PipelineUtils::makeColorBlendAttachmentState()
                    .setBlendEnable(true)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha))
                .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                    stateInfo.addAttributeDescriptions<Vertex>(0, 0);
                    stateInfo.addBindingDescriptions<Vertex>(0);
                }))
            .addShader({ "Resources/Shaders/bin/LightDebug_3D.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/LightDebug_3D.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mInput->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("LightDebugOverlay_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    SPtr<RHI::Image>        mInput;
    SPtr<RHI::Image>        mDepthBuffer;
    SPtr<RHI::Descriptor>   mSceneDescriptor;
    SceneGeometry*          mSceneGeometry;

    PushConstant            mPushConstants;
    SPtr<RHI::RenderPass>   mRenderPass;
    SPtr<RHI::Pipeline>     mPipeline;
};
