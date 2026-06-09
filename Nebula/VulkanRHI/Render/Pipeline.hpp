#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>
#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Descriptor.hpp"
#include "VulkanRHI/Detail/Resource.hpp"
#include "VulkanRHI/Raytracing/ShaderBindingTable.hpp"
#include "VulkanRHI/Rendering/VertexTraits.hpp"

namespace RHI
{
    /**
     * Struct to build a render pass and execute it on the spot.
     */
    struct Rendering
    {
        /**
         * Set the debug marker region label for the render pass body.
         */
        Rendering& setLabel(const std::string& value);

        /**
         * Set the render area with an extent, the offset will be set to {0, 0}.
         */
        Rendering& setRenderArea(const vk::Extent2D& extent);

        /**
         * Set the render area with a rect.
         */
        Rendering& setRenderArea(const vk::Rect2D& rect);

        /**
         * Add a color or depth attachment to the render pass.
         */
        Rendering& addAttachment(
            const SPtr<Image>&                   pImage,
            const vk::AttachmentLoadOp           loadOp     = vk::AttachmentLoadOp::eClear,
            const vk::AttachmentStoreOp          storeOp    = vk::AttachmentStoreOp::eStore,
            const std::optional<vk::ClearValue>& clearValue = std::nullopt,
            const SPtr<Image>&                   pResolve   = nullptr
        );

        /**
         * Get al the barriers required by the attachments for this render pass.
         */
        [[nodiscard]] Barrier getBarriers();

        /**
         * Insert all the barriers required for the attachments.
         */
        Rendering& insertBarriers(const CommandList* pCommandList);

        /**
         * Use the specified render area for viewport and scissor configuration.
         */
        Rendering& setViewportScissor(const CommandList* pCommandList);

        /**
         * Execute the render pass with the configured attachments with its body defined in a lambda.
         */
        template<std::invocable<CommandList*> Fn>
        void execute(CommandList* pCommandList, Fn&& lambda) const
        {
            pCommandList->beginLabel(mLabel);

            const auto renderingInfo = vk::RenderingInfo()
                .setRenderArea(mRenderArea)
                .setLayerCount(1)
                .setColorAttachments(mAttachmentInfos)
                .setPDepthAttachment(mDepthAttachmentInfo.has_value() ? &(*mDepthAttachmentInfo) : nullptr);
            pCommandList->getHandle().beginRendering(&renderingInfo);
            lambda(pCommandList);
            pCommandList->getHandle().endRendering();

            pCommandList->endLabel();
        }

    private:
        std::string                                mLabel;
        vk::Rect2D                                 mRenderArea;
        std::vector<Image*>                        mImages;
        std::vector<Image*>                        mResolveImages;
        std::vector<vk::RenderingAttachmentInfo>   mAttachmentInfos;
        std::optional<vk::RenderingAttachmentInfo> mDepthAttachmentInfo;
    };
}

namespace RHI
{
    // TODO: Move and rename the original.
    template <class T>
    concept VertexInput = VertexType<T>;

    /**
     * Information about a shader passed at creation time.
     */
    struct ShaderInfo2
    {
        std::filesystem::path   path;
        vk::ShaderStageFlagBits stage;
        const char*             entryPoint = "main";

        /**
         * From the specified shader file resolve the stage using standard
         * engine file extensions and return a ShaderInfo struct.
         */
        [[nodiscard]] static ShaderInfo2 fromFileName(const std::string& fileName);
    };

    enum class PipelineType2 : uint8_t
    {
        Unknown    = 0,
        Compute    = 1,
        Graphics   = 2,
        RayTracing = 3,
    };

    [[nodiscard]] constexpr std::string toString(const PipelineType2 type)
    {
        using enum PipelineType2;
        switch (type)
        {
            case Compute:       return "Compute";
            case Graphics:      return "Graphics";
            case RayTracing:    return "RayTracing";
            default:            return "Unknown";
        }
    }

    /**
     * Builder for common options between all pipeline types.
     */
    struct PipelineCommon
    {
        // Debug Label
        std::string label;

        PipelineCommon& setLabel(const std::string& value);

        /**
         * Collection of shaders used by the pipeline.
         */
        std::vector<ShaderInfo2> shaders;

        // Add shader by explicit ShaderInfo
        PipelineCommon& addShader(const ShaderInfo2& shaderInfo);

        // Add shader by file name and rely on automatic info resolution.
        PipelineCommon& addShader(const std::string& fileName);

        /**
         * Descriptor Set Layouts
         * - Bindings should start from 0 and form a valid sequence with no gaps / null descriptors.
         * - Adding descriptors at the same layout overwrites and keeps the last one.
         */
        std::map<uint32_t, Descriptor*> descriptors;

        // Add layout with explicit binding specification.
        PipelineCommon& addDescriptorLayout(const uint32_t binding, Descriptor* pDescriptor);

        /**
         * Push Constant
         * - Each pipeline can only define ONE push constant block.
         * - Must start at offset = 0 (enforced by setPushConstant())
         * - Size is resolved from template param.
         */
        std::optional<vk::PushConstantRange> pushConstantRange;

        template <class T>
        PipelineCommon& setPushConstant(const vk::ShaderStageFlags stages)
        {
            pushConstantRange = { stages, 0, sizeof(T) };
            return *this;
        }
    };

    /**
     * Builder for graphics pipeline state.
     */
    struct GraphicsPS
    {
        /**
         * Vertex Input Descriptions
         */
        std::vector<vk::VertexInputAttributeDescription> vtxInputAttributes;
        std::vector<vk::VertexInputBindingDescription>   vtxInputBindings;

        /**
         * Add a vertex input to the pipeline. They must be added in order of declaration,
         * the builder internally tracks the bindings & locations.
         * @tparam VertexT Vertex that satisfies the VertexInput concept.
         */
        template <VertexInput VertexT>
        GraphicsPS& addVertexType()
        {
            vtxInputAttributes.append_range(VertexT::getAttributes(vtxInputAttributes.size(), vtxInputBindings.size()));
            vtxInputBindings.push_back(VertexT::getBinding(vtxInputBindings.size()));
            return *this;
        }

        /**
         * Attachment formats and blend state
         */
        std::vector<vk::PipelineColorBlendAttachmentState>  blendStates         = {};
        std::vector<vk::Format>                             attachmentFormats   = {};
        vk::Format                                          depthFormat         = vk::Format::eUndefined;
        vk::Format                                          stencilFormat       = vk::Format::eUndefined;

        // Add an attachment format to the pipeline, color ones must be in order while depth and stencil are resolved automatically.
        GraphicsPS& addAttachmentFormat(const vk::Format value);

        // Add a default color attachment blend state "count" times.
        GraphicsPS& addDefaultAttachmentState(const uint32_t count = 1);

        // Add an alpha blending color attachment blend state "count" times.
        GraphicsPS& addAlphaAttachmentState(const uint32_t count = 1);

        // Add a custom blend state "count" times.
        GraphicsPS& addColorBlendAttachmentState(const vk::PipelineColorBlendAttachmentState& blendState, uint32_t count = 1);

        /**
         * Dynamic State
         */
        std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eScissor, vk::DynamicState::eViewport };

        /**
         * Graphics Pipeline State Configuration
         */
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = makeInputAssemblyState();
        vk::PipelineRasterizationStateCreateInfo rasterizationState = makeRasterizationState();
        vk::PipelineMultisampleStateCreateInfo   multisampleState   = makeMultisampleState();
        vk::PipelineDepthStencilStateCreateInfo  depthStencilState  = makeDepthStencilState();
        vk::PipelineViewportStateCreateInfo      viewportState      = makeViewportState();
        vk::PipelineDynamicStateCreateInfo       dynamicState       = makeDynamicState();
        vk::PipelineColorBlendStateCreateInfo    colorBlendState    = makeColorBlendState();
        vk::PipelineVertexInputStateCreateInfo   vertexInputState   = makeVertexInputState();

        GraphicsPS& setCullMode(const vk::CullModeFlagBits cullMode);

        GraphicsPS& setSampleCount(const vk::SampleCountFlagBits samples);

        GraphicsPS& setTopology(const vk::PrimitiveTopology topology);

        GraphicsPS& setWireframeMode(const bool value = true);

        /**
         * Freely configure the pipeline state via a lambda function.
         */
        template<std::invocable<GraphicsPS&> Fn>
        GraphicsPS& configure(Fn&& fn)
        {
            std::forward<Fn>(fn)(*this);
            return *this;
        }

    private:
        [[nodiscard]] static vk::PipelineInputAssemblyStateCreateInfo makeInputAssemblyState();

        [[nodiscard]] static vk::PipelineRasterizationStateCreateInfo makeRasterizationState();

        [[nodiscard]] static vk::PipelineMultisampleStateCreateInfo makeMultisampleState();

        [[nodiscard]] static vk::PipelineDepthStencilStateCreateInfo makeDepthStencilState();

        [[nodiscard]] static vk::PipelineViewportStateCreateInfo makeViewportState();

        [[nodiscard]] static vk::PipelineDynamicStateCreateInfo makeDynamicState();

        [[nodiscard]] static vk::PipelineColorBlendStateCreateInfo makeColorBlendState();

        [[nodiscard]] static vk::PipelineVertexInputStateCreateInfo makeVertexInputState();
    };

    /**
     * Builder for RT pipeline state.
     */
    struct RayTracingPS
    {
        uint32_t maxDepth = 1;

        RayTracingPS& setMaxDepth(const uint32_t value);
    };

    /**
     * Pipeline base class
     * TODO: Rename to "Pipeline"
     */
    class PipelineBase : public Resource
    {
    protected:
        // Used internally for pipeline creation
        struct LoadedShader
        {
            ShaderInfo2 shaderInfo;
            std::vector<char> code;
        };
    public:
        explicit PipelineBase(const PipelineCommon& common, const PipelineType2 type, const SPtr<Device>& device);

        ~PipelineBase() override;

        [[nodiscard]] const vk::Pipeline& getHandle() const noexcept;

        [[nodiscard]] const vk::PipelineLayout& getLayout() const noexcept;

        [[nodiscard]] PipelineType2 getType() const noexcept;

        [[nodiscard]] const vk::PipelineBindPoint& getBindPoint() const noexcept;

        [[nodiscard]] const std::optional<vk::PushConstantRange>& getPushConstantRange() const noexcept;

    protected:
        /**
         * Convert from pipeline type to Vulkan bind point.
         */
        [[nodiscard]] static vk::PipelineBindPoint toBindPoint(const PipelineType2 type);

        /**
         * Read a shader file from disk.
         */
        [[nodiscard]] static std::vector<char> readShaderFile(const std::filesystem::path& filePath);

        /**
         * Load a compiled SPIR-V shader from disk.
         */
        [[nodiscard]] static LoadedShader loadShader(const ShaderInfo2& shaderInfo);

        vk::Pipeline                         mPipeline          = VK_NULL_HANDLE;
        vk::PipelineLayout                   mPipelineLayout    = VK_NULL_HANDLE;

        PipelineType2                        mType              = PipelineType2::Unknown;
        vk::PipelineBindPoint                mBindPoint         = {};

        std::map<uint32_t, Descriptor*>      mDescriptors       = {};
        std::optional<vk::PushConstantRange> mPushConstantRange = std::nullopt;

    private:
        /**
         * Layout creation is pipeline type independent, so the base class ctor takes care of it.
         * Fills gaps in descriptor bindings with VK_NULL_HANDLE values.
         */
        void createPipelineLayout();
    };

    class ComputePipeline2 : public PipelineBase
    {
    public:
        explicit ComputePipeline2(const PipelineCommon& common, const SPtr<Device>& device)
        : PipelineBase(common, PipelineType2::Compute, device)
        {
            const auto computeShaders = common.shaders
                | std::views::filter([](const ShaderInfo2& info) {
                    return info.stage == vk::ShaderStageFlagBits::eCompute;
                })
                | std::ranges::to<std::vector>();
            if (computeShaders.empty())
            {
                exitWithError("No compute shader specified for pipeline [{}]", mLabel);
            }

            const auto shader = loadShader(computeShaders[0]);
            const auto moduleInfo = vk::ShaderModuleCreateInfo()
                .setCodeSize(sizeof(char) * shader.code.size())
                .setPCode(reinterpret_cast<const uint32_t*>(shader.code.data()));
            const auto stageInfo = vk::PipelineShaderStageCreateInfo()
                .setStage(shader.shaderInfo.stage)
                .setPName(shader.shaderInfo.entryPoint)
                .setPNext(&moduleInfo);

            const auto pipelineCreateInfo = vk::ComputePipelineCreateInfo()
                .setLayout(mPipelineLayout)
                .setStage(stageInfo);

            try
            {
                mPipeline = mDevice->getHandle().createComputePipeline(nullptr, pipelineCreateInfo).value;
                mDevice->nameObject<vk::Pipeline>({
                    .debugName = fmt::format("{}_Pipeline", mLabel),
                    .handle    = mPipeline,
                });
            }
            catch (const vk::SystemError& err)
            {
                exitWithError("Failed to create Pipeline [{}]: {}", mLabel, err.what());
            }
        }
    };

    class GraphicsPipeline2 : public PipelineBase
    {
    public:
        explicit GraphicsPipeline2(GraphicsPS ps, const PipelineCommon& common, const SPtr<Device>& device)
        : PipelineBase(common, PipelineType2::Graphics, device)
        {
            ps.colorBlendState.setAttachments(ps.blendStates);
            ps.vertexInputState.setVertexAttributeDescriptions(ps.vtxInputAttributes);
            ps.vertexInputState.setVertexBindingDescriptions(ps.vtxInputBindings);
            ps.dynamicState.setDynamicStates(ps.dynamicStates);

            std::vector<LoadedShader> shaders;
            shaders.reserve(common.shaders.size());
            for (const auto& shaderInfo : common.shaders)
            {
                shaders.push_back(loadShader(shaderInfo));
            }

            std::vector<vk::PipelineShaderStageCreateInfo> stageInfos(shaders.size());
            std::vector<vk::ShaderModuleCreateInfo>        moduleInfos(shaders.size());
            for (const auto& [i, shader] : nbl::enumerate(shaders))
            {
                moduleInfos[i] = vk::ShaderModuleCreateInfo()
                    .setCodeSize(sizeof(char) * shader.code.size())
                    .setPCode(reinterpret_cast<const uint32_t*>(shader.code.data()));

                stageInfos[i] = vk::PipelineShaderStageCreateInfo()
                    .setStage(shader.shaderInfo.stage)
                    .setPName(shader.shaderInfo.entryPoint)
                    .setPNext(&moduleInfos[i]);
            }

            const auto renderingInfo = vk::PipelineRenderingCreateInfo()
                .setColorAttachmentFormats(ps.attachmentFormats)
                .setDepthAttachmentFormat(ps.depthFormat)
                .setStencilAttachmentFormat(ps.stencilFormat);

            auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
                .setPInputAssemblyState(&ps.inputAssemblyState)
                .setPRasterizationState(&ps.rasterizationState)
                .setPMultisampleState(&ps.multisampleState)
                .setPDepthStencilState(&ps.depthStencilState)
                .setPViewportState(&ps.viewportState)
                .setPDynamicState(&ps.dynamicState)
                .setPColorBlendState(&ps.colorBlendState)
                .setPVertexInputState(&ps.vertexInputState)
                .setStages(stageInfos)
                .setLayout(mPipelineLayout)
                .setPNext(&renderingInfo);

            try
            {
                mPipeline = mDevice->getHandle().createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;
                mDevice->nameObject<vk::Pipeline>({
                    .debugName = mLabel,
                    .handle    = mPipeline,
                });
            }
            catch (const vk::SystemError& err)
            {
                exitWithError("Failed to create Pipeline [{}]: {}", mLabel, err.what());
            }
        }
    };

    class RayTracingPipeline2 : public PipelineBase
    {
    public:
        explicit RayTracingPipeline2(const RayTracingPS& ps, const PipelineCommon& common, const SPtr<Device>& device)
        : PipelineBase(common, PipelineType2::RayTracing, device)
        , mMaxDepth(ps.maxDepth)
        {
            using enum vk::ShaderStageFlagBits;
            static std::set rtStages = {
                eRaygenKHR, eClosestHitKHR, eMissKHR, eAnyHitKHR, eCallableKHR, eIntersectionKHR
            };

            ShaderBindingTableCreateInfo sbtCreateInfo = {};
            std::vector<LoadedShader> shaders;
            bool hasRaygen = false;
            for (const auto& shaderInfo : common.shaders)
            {
                if (!rtStages.contains(shaderInfo.stage))
                {
                    continue;
                }

                switch (shaderInfo.stage)
                {
                    case eRaygenKHR:
                    {
                        if (hasRaygen)
                        {
                            exitWithError("A Ray Tracing Pipeline can only have one RayGen shader.");
                        }
                        hasRaygen = true;
                        break;
                    }
                    case eClosestHitKHR:
                    {
                        sbtCreateInfo.hitCount++;
                        break;
                    }
                    case eMissKHR:
                    {
                        sbtCreateInfo.missCount++;
                        break;
                    }
                    case eCallableKHR:
                    {
                        sbtCreateInfo.callableCount++;
                        break;
                    }
                    default:
                    {
                        spdlog::warn("Shader type {} is not supported.", vk::to_string(shaderInfo.stage));
                        break;
                    }
                }

                shaders.push_back(loadShader(shaderInfo));
            }

            if (!hasRaygen)
            {
                exitWithError("A Ray Tracing Pipeline must have a RayGen shader");
            }

            std::vector<vk::PipelineShaderStageCreateInfo> stageInfos(shaders.size());
            std::vector<vk::ShaderModuleCreateInfo>        moduleInfos(shaders.size());
            for (const auto& [i, shader] : nbl::enumerate(shaders))
            {
                moduleInfos[i] = vk::ShaderModuleCreateInfo()
                    .setCodeSize(sizeof(char) * shader.code.size())
                    .setPCode(reinterpret_cast<const uint32_t*>(shader.code.data()));

                stageInfos[i] = vk::PipelineShaderStageCreateInfo()
                    .setStage(shader.shaderInfo.stage)
                    .setPName(shader.shaderInfo.entryPoint)
                    .setPNext(&moduleInfos[i]);
            }
            const auto shaderGroups = createShaderGroups(stageInfos);

            const auto pipelineCreateInfo = vk::RayTracingPipelineCreateInfoKHR()
            .setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR | vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR)
                .setStages(stageInfos)
                .setGroupCount(shaderGroups.size())
                .setPGroups(shaderGroups.data())
                .setMaxPipelineRayRecursionDepth(mMaxDepth)
                .setLayout(mPipelineLayout);

            const vk::Result result = mDevice->getHandle().createRayTracingPipelinesKHR(nullptr, nullptr, 1, &pipelineCreateInfo, nullptr, &mPipeline);
            if (result != vk::Result::eSuccess)
            {
                exitWithError("Failed to create RT pipeline: {}", vk::to_string(result));
            }

            mDevice->nameObject<vk::Pipeline>({
                .debugName = mLabel,
                .handle    = mPipeline,
            });

            sbtCreateInfo.pipeline  = mPipeline;
            sbtCreateInfo.device    = mDevice;
            sbtCreateInfo.debugName = std::format("{}_ShaderBindingTable", mLabel);

            mShaderBindingTable = ShaderBindingTable::create(sbtCreateInfo);
        }

        [[nodiscard]] ShaderBindingTable* getShaderBindingTable() const { return mShaderBindingTable.get(); }

        [[nodiscard]] uint32_t getMaxDepth() const noexcept { return mMaxDepth; }

    private:
        [[nodiscard]] std::vector<vk::RayTracingShaderGroupCreateInfoKHR> createShaderGroups(const std::vector<vk::PipelineShaderStageCreateInfo>& stageCreateInfos) const
        {
            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> result;
            for (auto i = 0; i < stageCreateInfos.size(); i++)
            {
                switch (stageCreateInfos[i].stage)
                {
                    case vk::ShaderStageFlagBits::eRaygenKHR:
                    case vk::ShaderStageFlagBits::eMissKHR: {
                        auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                            .setGeneralShader(i)
                            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                        result.push_back(group);
                        break;
                    }
                    case vk::ShaderStageFlagBits::eClosestHitKHR: {
                        auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
                            .setGeneralShader(VK_SHADER_UNUSED_KHR)
                            .setClosestHitShader(i)
                            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                        result.push_back(group);
                        break;
                    }
                    case vk::ShaderStageFlagBits::eCallableKHR: {
                        auto group = vk::RayTracingShaderGroupCreateInfoKHR()
                            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                            .setGeneralShader(i)
                            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
                            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
                            .setIntersectionShader(VK_SHADER_UNUSED_KHR);
                        result.push_back(group);
                        break;
                    }
                    default: {
                        spdlog::warn("Non-Raytracing shader ({}) specified for Pipeline {}", to_string(stageCreateInfos[i].stage), mLabel);
                    }
                }
            }
            return result;
        }

        uint32_t                 mMaxDepth;
        SPtr<ShaderBindingTable> mShaderBindingTable;
    };
}
