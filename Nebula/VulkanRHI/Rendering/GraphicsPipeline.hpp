#pragma once

#include <vulkan/vulkan.hpp>

#include "Pipeline.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct GraphicsPipelineStateInfo
    {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineUtils::makeInputAssemblyState();
        vk::PipelineRasterizationStateCreateInfo rasterizationState = PipelineUtils::makeRasterizationState();
        vk::PipelineMultisampleStateCreateInfo   multisampleState   = PipelineUtils::makeMultisampleState();
        vk::PipelineDepthStencilStateCreateInfo  depthStencilState  = PipelineUtils::makeDepthStencilState();
        vk::PipelineViewportStateCreateInfo      viewportState      = PipelineUtils::makeViewportState();
        vk::PipelineDynamicStateCreateInfo       dynamicState       = PipelineUtils::makeDynamicState();
        vk::PipelineColorBlendStateCreateInfo    colorBlendState    = PipelineUtils::makeColorBlendState();
        vk::PipelineVertexInputStateCreateInfo   vertexInputState   = PipelineUtils::makeVertexInputState();

        std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};

        std::vector<vk::VertexInputAttributeDescription>    attributeDescriptions;
        std::vector<vk::VertexInputBindingDescription>      bindingDescriptions;
        std::vector<vk::PipelineColorBlendAttachmentState>  attachmentStates;

        void update()
        {
            colorBlendState.setAttachmentCount(static_cast<uint32_t>(attachmentStates.size()));
            colorBlendState.setPAttachments(attachmentStates.data());

            vertexInputState.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()));
            vertexInputState.setPVertexAttributeDescriptions(attributeDescriptions.data());

            vertexInputState.setVertexBindingDescriptionCount(static_cast<uint32_t>(bindingDescriptions.size()));
            vertexInputState.setPVertexBindingDescriptions(bindingDescriptions.data());

            dynamicState.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()));
            dynamicState.setPDynamicStates(dynamicStates.data());
        }

        template <class T>
        GraphicsPipelineStateInfo& addAttributeDescriptions(uint32_t base_location = 0, uint32_t binding = 0)
        {
            for (const auto& attribute : T::attribute_descriptions(base_location, binding))
            {
                attributeDescriptions.push_back(attribute);
            }
            return *this;
        }

        template <class T>
        GraphicsPipelineStateInfo& addBindingDescriptions(uint32_t binding = 0)
        {
            bindingDescriptions.push_back(T::binding_description(binding));
            return *this;
        }

        GraphicsPipelineStateInfo& setCullMode(const vk::CullModeFlagBits cullMode)
        {
            rasterizationState.setCullMode(cullMode);
            return *this;
        }

        GraphicsPipelineStateInfo& setWireframeMode(const bool value = true)
        {
            rasterizationState.setPolygonMode(value ? vk::PolygonMode::eFill : vk::PolygonMode::eLine);
            return *this;
        }

        GraphicsPipelineStateInfo& configure(const std::function<void(GraphicsPipelineStateInfo&)>& fn)
        {
            fn(*this);
            update();
            return *this;
        }

        GraphicsPipelineStateInfo& addAttachmentState(const vk::PipelineColorBlendAttachmentState& state = PipelineUtils::makeColorBlendAttachmentState())
        {
            attachmentStates.push_back(state);
            return *this;
        }
    };

    begin_PipelineCreateInfoStruct(Graphics)
        std::map<vk::ShaderStageFlagBits, ShaderInfo> shaderInfos;
        GraphicsPipelineStateInfo                     stateInfo = {};

        GraphicsPipelineCreateInfo& setStateInfo(const GraphicsPipelineStateInfo& value)
        {
            stateInfo = value;
            return *this;
        }

        GraphicsPipelineCreateInfo& addShader(const ShaderInfo& shaderInfo)
        {
            shaderInfos[shaderInfo.shaderStage] = shaderInfo;
            return *this;
        }
    end_PipelineCreateInfoStruct;

    class GraphicsPipeline final : public Pipeline
    {
    public:
        nbl_DISABLE_COPY(GraphicsPipeline);
        explicit GraphicsPipeline(GraphicsPipelineCreateInfo createInfo);

        static UPtr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        ~GraphicsPipeline() override = default;

    private:
        PipelineAttachmentInfo  mAttachments;
    };
}