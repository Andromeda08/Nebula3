#pragma once

#include "GeometrySystem.hpp"
#include "Cleanup/Scene.hpp"

namespace nbl
{
    class GeometrySystemDebugRenderPass
    {
        struct PushConstants
        {
            glm::mat4       model;
            GeometryIndex   geometryIndex;
        };
    public:
        GeometrySystemDebugRenderPass(const SPtr<RHI::VulkanRHI>& rhi, Scene* pScene, GeometrySystem* pGeometrySystem, const RHI::Descriptor* pDescriptor)
        : mRHI(rhi)
        , mGeometrySystem(pGeometrySystem)
        , mScene(pScene)
        {
            mOutput = mRHI->createImage({
                .extent     = mRHI->getSwapchain()->getProperties().extent,
                .format     = vk::Format::eR32G32B32A32Sfloat,
                .usageFlags = vk::ImageUsageFlagBits::eColorAttachment,
                .debugName  = "GeometrySystemDebugRenderPass_GeometryIndex",
            });
            mDepth = mRHI->createImage({
                .extent     = mRHI->getSwapchain()->getProperties().extent,
                .format     = vk::Format::eD32Sfloat,
                .usageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                .debugName  = "GeometrySystemDebugRenderPass_DepthBuffer",
            });

            mRenderPass = mRHI->createRenderPass({
                .renderArea = {{0,0}, mRHI->getSwapchain()->getProperties().extent},
                .colorAttachments = {
                    RHI::Attachment {
                        .image = mOutput->getImage(),
                        .attachmentInfo = vk::RenderingAttachmentInfo()
                            .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                            .setImageView(mOutput->getImageView())
                            .setLoadOp(vk::AttachmentLoadOp::eClear)
                            .setStoreOp(vk::AttachmentStoreOp::eStore)
                    },
                },
                .depthAttachment  = RHI::Attachment {
                    .image = mDepth->getImage(),
                    .attachmentInfo = vk::RenderingAttachmentInfo()
                            .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                            .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                            .setImageView(mDepth->getImageView())
                            .setLoadOp(vk::AttachmentLoadOp::eClear)
                            .setStoreOp(vk::AttachmentStoreOp::eStore),
                },
                .label = "GeometrySystemDebugRenderPass_RenderPass",
            });

            const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
                .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants) })
                .addDescriptorSetLayout(pDescriptor->getLayout())
                .setStateInfo(RHI::GraphicsPipelineStateInfo()
                    .addDefaultAttachmentStates(1)
                    .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                        stateInfo.addAttributeDescriptions<Vertex>(0, 0);
                        stateInfo.addBindingDescriptions<Vertex>(0);
                    }))
                .addShader({ Configuration::getShaderFilePath("GeoSysDebug.vert.spv").string(), vk::ShaderStageFlagBits::eVertex })
                .addShader({ Configuration::getShaderFilePath("GeoSysDebug.frag.spv").string(), vk::ShaderStageFlagBits::eFragment })
                .addColorAttachmentFormat(mOutput->getProperties().format)
                .setDepthAttachmentFormat(vk::Format::eD32Sfloat)
                .setDebugName("GeometrySystemDebugRenderPass_Pipeline");

            mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
        }

        void execute(const RHI::CommandList* pCommandList, const vk::DescriptorSet& descriptor)
        {
            pCommandList->beginLabel("GeometrySystemDebug_RenderPass");

            RHI::Barrier()
                .addBarrier(mOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
                .addBarrier(mDepth->getBarrier(RHI::ImageUsage::DepthAttachment))
                .insert(pCommandList);

            const auto& metadata = mGeometrySystem->mMetadata;
            if (metadata.empty())
            {
                return;
            }

            mRenderPass->execute(pCommandList, [&](const RHI::CommandList* commandList) -> void
            {
                mPipeline->bind(commandList);
                mPipeline->bindDescriptorSet(commandList, descriptor);

                commandList->getHandle().bindVertexBuffers(0, mGeometrySystem->mVertexBuffer->getHandle(), {0});
                commandList->getHandle().bindIndexBuffer(mGeometrySystem->mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);

                for (const auto& obj : mScene->getObjects())
                {
                    const PushConstants pc = {
                        .model         = obj->mTransform.getModel(),
                        .geometryIndex = obj->mGeometryIndex,
                    };

                    const auto& meta = mGeometrySystem->getGeometryMeta(obj->mGeometryIndex);

                    mPipeline->pushConstants(commandList, &pc);

                    commandList->getHandle().drawIndexed(
                        meta.indexCount,
                        1,
                        meta.firstIndex,
                        static_cast<int32_t>(meta.firstVertex),
                        0
                    );
                }
            });

            pCommandList->endLabel();
        }

    private:
        SPtr<RHI::VulkanRHI>    mRHI;
        GeometrySystem*         mGeometrySystem = nullptr;
        Scene*                  mScene = nullptr;

        SPtr<RHI::Pipeline>     mPipeline;
        SPtr<RHI::Pipeline>     mMeshShader;
        SPtr<RHI::RenderPass>   mRenderPass;
        SPtr<RHI::Image>        mOutput;
        SPtr<RHI::Image>        mDepth;
    };
}
