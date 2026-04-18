#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "Shader.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/Commands/CommandList.hpp"

namespace RHI
{
    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Mesh,
        Task,
    };

    constexpr vk::ShaderStageFlagBits toVulkanStage(const ShaderStage& stage)
    {
        using enum ShaderStage;
        switch (stage)
        {
            case Vertex:    return vk::ShaderStageFlagBits::eVertex;
            case Fragment:  return vk::ShaderStageFlagBits::eFragment;
            case Mesh:      return vk::ShaderStageFlagBits::eMeshEXT;
            case Task:      return vk::ShaderStageFlagBits::eTaskEXT;
        }
        std::unreachable();
    }

    enum class PipelineType
    {
        Graphics,
        Compute,
        RayTracing,
    };

    struct PipelineAttachmentInfo
    {
        std::vector<vk::Format> colorAttachmentFormats  = {};
        vk::Format              depthFormat             = vk::Format::eUndefined;
        vk::Format              stencilFormat           = vk::Format::eUndefined;
    };

    #define begin_PipelineCreateInfoStruct(T)                                                       \
        struct T##PipelineCreateInfo                                                                \
        {                                                                                           \
            vk::PushConstantRange                   pushConstantRange       = {};                   \
            std::vector<vk::DescriptorSetLayout>    descriptorSetLayouts    = {};                   \
            PipelineAttachmentInfo                  attachmentInfo          = {};                   \
            std::string                             debugName               = "Unknown Pipeline";   \
            vk::RenderPass                          renderPass              = nullptr;              \
            SPtr<Device>                            device                  = nullptr;              \
            template <class T> \
            T##PipelineCreateInfo& setPushConstantRange(const vk::ShaderStageFlags stages) {        \
                pushConstantRange = { stages, 0, sizeof(T) };                                       \
                return* this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& setPushConstantRange(const vk::PushConstantRange& value) {       \
                pushConstantRange = value;                                                          \
                return* this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& addDescriptorSetLayout(const vk::DescriptorSetLayout& layout) {  \
                descriptorSetLayouts.push_back(layout);                                             \
                return *this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& addColorAttachmentFormat(const vk::Format format) {              \
                attachmentInfo.colorAttachmentFormats.push_back(format);                            \
                return *this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& addColorAttachmentFormats(const std::vector<vk::Format>& fmts) { \
                attachmentInfo.colorAttachmentFormats.append_range(fmts);                           \
                return *this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& setDepthAttachmentFormat(const vk::Format format) {              \
                attachmentInfo.depthFormat = format;                                                \
                return *this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& setDebugName(const std::string& string) {                        \
                debugName = string;                                                                 \
                return *this;                                                                       \
            }                                                                                       \
            T##PipelineCreateInfo& setDevice(const SPtr<Device>& d) {                               \
                device = d;                                                                         \
                return *this;                                                                       \
            }

    #define end_PipelineCreateInfoStruct };

    class Pipeline
    {
    public:
        virtual ~Pipeline();

        [[deprecated("Use the version that takes in a CommandList as parameter")]]
        void bind(const vk::CommandBuffer& commandBuffer) const;

        void bind(const CommandList* pCommandList) const;

        [[deprecated("Use the version that takes in a CommandList as parameter")]]
        void bindDescriptorSet(const vk::CommandBuffer& commandBuffer, const vk::DescriptorSet& descriptorSet) const;

        /**
         * Bind a DescriptorSet at the specified index, or 0 when omitted.
         */
        void bindDescriptorSet(const CommandList* pCommandList, const vk::DescriptorSet& descriptorSet, uint32_t setIndex = 0) const;

        [[deprecated("Use the version that takes in a CommandList as parameter")]]
        void bindDescriptorSets(const vk::CommandBuffer& commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets) const;

        /**
         * Bind multiple DescriptorSets starting at the specified index, or 0 when omitted.
         */
        void bindDescriptorSets(const CommandList* pCommandList, const std::vector<vk::DescriptorSet>& descriptorSets, uint32_t firstSet = 0) const;

        [[deprecated("Use the version that takes in a CommandList as parameter")]]
        void pushConstants(const vk::CommandBuffer& commandBuffer, const void* pData) const;

        void pushConstants(const CommandList* pCommandList, const void* pData) const;

        [[nodiscard]] const vk::Pipeline& getHandle() const noexcept;

        [[nodiscard]] const vk::PipelineLayout& getPipelineLayout() const noexcept;

    protected:
        vk::Pipeline            mPipeline;
        vk::PipelineLayout      mPipelineLayout;

        vk::PushConstantRange   mPushConstantRange;

        vk::PipelineBindPoint   mBindPoint {};
        PipelineType            mPipelineType {};

        SPtr<Device>            mDevice;
    };

    struct PipelineUtils
    {
        static constexpr vk::PipelineBindPoint pipelineTypeToBindPoint(const PipelineType type)
        {
            using enum PipelineType;
            switch (type)
            {
                case Compute:    return vk::PipelineBindPoint::eCompute;
                case RayTracing: return vk::PipelineBindPoint::eRayTracingKHR;
                default:         return vk::PipelineBindPoint::eGraphics;
            }
        }

        static vk::PipelineInputAssemblyStateCreateInfo makeInputAssemblyState()
        {
            return vk::PipelineInputAssemblyStateCreateInfo()
                .setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(false)
                .setFlags({})
                .setPNext(nullptr);
        }

        static vk::PipelineRasterizationStateCreateInfo makeRasterizationState()
        {
            return vk::PipelineRasterizationStateCreateInfo()
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setDepthClampEnable(false)
                .setDepthBiasEnable(false)
                .setDepthBiasClamp(0.0f)
                .setDepthBiasSlopeFactor(0.0f)
                .setLineWidth(1.0f)
                .setRasterizerDiscardEnable(false)
                .setPNext(nullptr);
        }

        static vk::PipelineMultisampleStateCreateInfo makeMultisampleState()
        {
            return vk::PipelineMultisampleStateCreateInfo()
                .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                .setSampleShadingEnable(false)
                .setPSampleMask(nullptr)
                .setAlphaToCoverageEnable(false)
                .setAlphaToOneEnable(false)
                .setPNext(nullptr);
        }

        static vk::PipelineDepthStencilStateCreateInfo makeDepthStencilState()
        {
            return vk::PipelineDepthStencilStateCreateInfo()
                .setDepthTestEnable(true)
                .setDepthWriteEnable(true)
                .setDepthCompareOp(vk::CompareOp::eLess)
                .setDepthBoundsTestEnable(false)
                .setStencilTestEnable(false);
        }

        static vk::PipelineViewportStateCreateInfo makeViewportState()
        {
            return vk::PipelineViewportStateCreateInfo()
                .setViewportCount(1)
                .setPViewports(nullptr)
                .setScissorCount(1)
                .setPScissors(nullptr)
                .setPNext(nullptr);
        }

        static vk::PipelineDynamicStateCreateInfo makeDynamicState()
        {
            return vk::PipelineDynamicStateCreateInfo()
                .setDynamicStateCount(0)
                .setPDynamicStates(nullptr)
                .setPNext(nullptr);
        }

        static vk::PipelineColorBlendStateCreateInfo makeColorBlendState()
        {
            return vk::PipelineColorBlendStateCreateInfo()
                .setLogicOp(vk::LogicOp::eClear)
                .setLogicOpEnable(false)
                .setAttachmentCount(0)
                .setPAttachments(nullptr)
                .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f})
                .setPNext(nullptr);
        }

        static vk::PipelineVertexInputStateCreateInfo makeVertexInputState()
        {
            return vk::PipelineVertexInputStateCreateInfo()
                .setVertexAttributeDescriptionCount(0)
                .setPVertexAttributeDescriptions(nullptr)
                .setVertexBindingDescriptionCount(0)
                .setPVertexBindingDescriptions(nullptr)
                .setPNext(nullptr);
        }

        using Clr = vk::ColorComponentFlagBits;

        static vk::PipelineColorBlendAttachmentState makeColorBlendAttachmentState(
            const vk::ColorComponentFlags colorWriteMask      = Clr::eR | Clr::eG | Clr::eB | Clr::eA,
            const vk::Bool32              blendEnable         = false,
            const vk::BlendFactor         srcColorBlendFactor = vk::BlendFactor::eOne,
            const vk::BlendFactor         dstColorBlendFactor = vk::BlendFactor::eZero,
            const vk::BlendOp             colorBlendOp        = vk::BlendOp::eAdd,
            const vk::BlendFactor         srcAlphaBlendFactor = vk::BlendFactor::eOne,
            const vk::BlendFactor         dstAlphaBlendFactor = vk::BlendFactor::eZero,
            const vk::BlendOp             alphaBlendOp        = vk::BlendOp::eAdd)
        {
            return vk::PipelineColorBlendAttachmentState()
                .setColorWriteMask(colorWriteMask)
                .setBlendEnable(blendEnable)
                .setSrcColorBlendFactor(srcColorBlendFactor)
                .setDstColorBlendFactor(dstColorBlendFactor)
                .setColorBlendOp(colorBlendOp)
                .setSrcAlphaBlendFactor(srcAlphaBlendFactor)
                .setDstAlphaBlendFactor(dstAlphaBlendFactor)
                .setAlphaBlendOp(alphaBlendOp);
        }
    };
}
