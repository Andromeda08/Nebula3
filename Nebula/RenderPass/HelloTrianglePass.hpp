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

        RHI::GraphicsPipelineCreateInfo createInfo;
        createInfo.addShader({ "Resources/Shaders/bin/HelloTriangle.vert.spv", "main", vk::ShaderStageFlagBits::eVertex });
        createInfo.addShader({ "Resources/Shaders/bin/HelloTriangle.frag.spv", "main", vk::ShaderStageFlagBits::eFragment });
        createInfo.attachmentInfo.colorAttachmentFormats.push_back(rhi->getSwapchain()->getProperties().format);
        createInfo.debugName = "HelloTriangle";
        createInfo.stateInfo = RHI::GraphicsPipelineStateInfo()
            .setCullMode(vk::CullModeFlagBits::eNone);
        createInfo.stateInfo.attachmentStates.push_back( RHI::PipelineUtils::makeColorBlendAttachmentState());
        createInfo.stateInfo.update();

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
