#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "Shader.hpp"
#include "VulkanRHI/Device.hpp"

namespace RHI
{
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

        void bind(const vk::CommandBuffer& commandBuffer) const;

        void bindDescriptorSet(const vk::CommandBuffer& commandBuffer, const vk::DescriptorSet& descriptorSet) const;

        void bindDescriptorSets(const vk::CommandBuffer& commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets) const;

        void pushConstants(const vk::CommandBuffer& commandBuffer, const void* pData) const;

        vk::Pipeline getHandle() const
        {
            return mPipeline;
        }

        vk::PipelineLayout getPipelineLayout() const
        {
            return mPipelineLayout;
        }

        [[nodiscard]] vk::PipelineBindPoint getBindPoint() const noexcept { return mBindPoint; }

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
