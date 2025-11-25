#pragma once

#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class HelloTrianglePass
{
public:
    explicit HelloTrianglePass(const SPtr<RHI::VulkanRHI>& rhi)
    {
        const auto attachment = RHI::Attachment {
            .image = rhi->getSwapchain()->getImage(0),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(rhi->getSwapchain()->getImageView(0))
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        mRenderPass = rhi->createRenderPass({
            .renderArea       = rhi->getSwapchain()->getProperties().area,
            .colorAttachments = { attachment },
            .label            = "HelloTriangle",
        });

        RHI::GraphicsPipelineCreateInfo createInfo = RHI::GraphicsPipelineCreateInfo()
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/HelloTriangle.vert.spv", "main", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/HelloTriangle.frag.spv", "main", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(rhi->getSwapchain()->getProperties().format)
            .setDebugName("HelloTriangle");

        mPipeline = rhi->createGraphicsPipeline(createInfo);

        mSwapchain = rhi->getSwapchain();
    }

    void execute(const vk::CommandBuffer& commandBuffer, const RHI::FrameData& frameData)
    {
        const auto attachment = RHI::Attachment {
            .image = mSwapchain->getImage(frameData.acquiredIndex),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mSwapchain->getImageView(frameData.acquiredIndex))
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };

        mRenderPass->setColorAttachment(0, attachment);

        mPipeline->bind(commandBuffer);
        mRenderPass->execute(commandBuffer, [](const vk::CommandBuffer& cmd){
            cmd.draw(3, 1, 0, 0);
        });
    }

private:
    RHI::Swapchain*             mSwapchain;
    UPtr<RHI::RenderPass>       mRenderPass;
    UPtr<RHI::GraphicsPipeline> mPipeline;
};
