#include "GraphicsPipeline.hpp"

namespace RHI
{
    GraphicsPipeline::GraphicsPipeline(GraphicsPipelineCreateInfo createInfo)
    : IPipeline()
    , mPushConstantRange(createInfo.pushConstantRange)
    , mAttachments(createInfo.attachmentInfo)
    , mDevice(createInfo.device)
    {
        assert(!createInfo.shaderInfos.empty());

        auto& pipelineState = createInfo.stateInfo;
        pipelineState.update();

        const auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(createInfo.descriptorSetLayouts.size())
            .setPSetLayouts(createInfo.descriptorSetLayouts.data())
            .setPushConstantRangeCount(mPushConstantRange.size != 0 ? 1 : 0)
            .setPPushConstantRanges(mPushConstantRange.size != 0 ? &mPushConstantRange : nullptr);

        mPipelineLayout = mDevice->getHandle().createPipelineLayout(layoutCreateInfo);
        mDevice->nameObject<vk::PipelineLayout>({
            .debugName = std::format("{} Layout", createInfo.debugName),
            .handle    = mPipelineLayout,
        });

        const auto shaders = Shader::compileShaders(mDevice.get(), createInfo.shaderInfos);
        const auto shaderStageInfos = getStageCreateInfos(shaders);

        auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
            .setPInputAssemblyState(&pipelineState.inputAssemblyState)
            .setPRasterizationState(&pipelineState.rasterizationState)
            .setPMultisampleState(&pipelineState.multisampleState)
            .setPDepthStencilState(&pipelineState.depthStencilState)
            .setPViewportState(&pipelineState.viewportState)
            .setPDynamicState(&pipelineState.dynamicState)
            .setPColorBlendState(&pipelineState.colorBlendState)
            .setPVertexInputState(&pipelineState.vertexInputState)
            .setStageCount(shaderStageInfos.size())
            .setPStages(shaderStageInfos.data())
            .setLayout(mPipelineLayout)
            .setRenderPass(createInfo.renderPass)
            .setPNext(nullptr);

        vk::PipelineRenderingCreateInfo renderingInfo;
        if (!createInfo.renderPass)
        {
            renderingInfo = vk::PipelineRenderingCreateInfo()
                .setColorAttachmentCount(mAttachments.colorAttachmentFormats.size())
                .setPColorAttachmentFormats(mAttachments.colorAttachmentFormats.data())
                .setDepthAttachmentFormat(mAttachments.depthFormat)
                .setStencilAttachmentFormat(mAttachments.stencilFormat);

            graphicsPipelineCreateInfo
                .setRenderPass(nullptr)
                .setPNext(&renderingInfo);
        }

        mPipeline = mDevice->getHandle().createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;
        mDevice->nameObject<vk::Pipeline>({
            .debugName = createInfo.debugName,
            .handle    = mPipeline,
        });
    }

    UPtr<GraphicsPipeline> GraphicsPipeline::create(GraphicsPipelineCreateInfo createInfo)
    {
        return std::make_unique<GraphicsPipeline>(createInfo);
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        mDevice->getHandle().destroyPipeline(mPipeline);
        mDevice->getHandle().destroyPipelineLayout(mPipelineLayout);
    }

    void GraphicsPipeline::bind(const vk::CommandBuffer& commandBuffer)
    {
        commandBuffer.bindPipeline(sBindPoint, mPipeline);
    }

    void GraphicsPipeline::bindDescriptorSet(const vk::CommandBuffer& commandBuffer, const vk::DescriptorSet& descriptorSet)
    {
        commandBuffer.bindDescriptorSets(sBindPoint, mPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    void GraphicsPipeline::bindDescriptorSets(const vk::CommandBuffer& commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets)
    {
        commandBuffer.bindDescriptorSets(sBindPoint, mPipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
    }

    void GraphicsPipeline::pushConstants(const vk::CommandBuffer& commandBuffer, const void* pData) const
    {
        commandBuffer.pushConstants(mPipelineLayout, mPushConstantRange.stageFlags, mPushConstantRange.offset, mPushConstantRange.size, pData);
    }
}
