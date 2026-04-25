#pragma once

#include "Pass.hpp"
#include "RenderGraph/Node.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

class HelloTrianglePass final : public Pass
{
public:
    explicit HelloTrianglePass(const SPtr<RHI::VulkanRHI>& rhi)
    : Pass()
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
            .addShader({ "Resources/Shaders/bin/HelloTriangle.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/HelloTriangle.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(rhi->getSwapchain()->getProperties().format)
            .setDebugName("HelloTriangle");

        mPipeline = rhi->createGraphicsPipeline(createInfo);

        mSwapchain = rhi->getSwapchain();
    }

    ~HelloTrianglePass() override = default;

    void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) override
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

        mPipeline->bind(commandList->getHandle());
        mRenderPass->execute(commandList->getHandle(), [](const vk::CommandBuffer& cmd){
            cmd.draw(3, 1, 0, 0);
        });
    }

    static rg::NodeCreateInfo getNodeInfo()
    {
        return {
            .nodeType     = rg::NodeType::HelloTrianglePresent,
            .displayName  = "Hello Triangle",
            .dependencies = {
                rg::DependencyInfo {
                    .name           = "Scene Data",
                    .dependencyType = rg::DependencyType::Read,
                    .resourceType   = rg::ResourceType::SceneData,
                },
            },
        };
    }

private:
    RHI::Swapchain*             mSwapchain;
    UPtr<RHI::RenderPass>       mRenderPass;
    UPtr<RHI::GraphicsPipeline> mPipeline;
};
