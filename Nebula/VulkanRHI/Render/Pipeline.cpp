#include "Pipeline.hpp"

/**
 * Rendering
 */
namespace RHI
{
    Rendering& Rendering::setLabel(const std::string& value)
    {
        mLabel = value;
        return *this;
    }

    Rendering& Rendering::setRenderArea(const vk::Extent2D& extent)
    {
        mRenderArea = vk::Rect2D().setExtent(extent).setOffset({0, 0});
        return *this;
    }

    Rendering& Rendering::setRenderArea(const vk::Rect2D& rect)
    {
        mRenderArea = rect;
        return *this;
    }

    Rendering& Rendering::addAttachment(
        const SPtr<Image>& pImage, const vk::AttachmentLoadOp loadOp, const vk::AttachmentStoreOp storeOp,
        const std::optional<vk::ClearValue>& clearValue, const SPtr<Image>& pResolve)
    {
        const bool isDepth     = vk::hasDepthComponent(pImage->getProperties().format);
        const auto _clearValue = clearValue.value_or(isDepth
            ? vk::ClearValue().setDepthStencil({1.0f, 0})
            : vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}));
        const auto _layout = isDepth
            ? vk::ImageLayout::eDepthAttachmentOptimal
            : vk::ImageLayout::eColorAttachmentOptimal;

        auto attachmentInfo = vk::RenderingAttachmentInfo()
            .setClearValue(_clearValue)
            .setImageLayout(_layout)
            .setImageView(pImage->getImageView())
            .setLoadOp(loadOp)
            .setStoreOp(pResolve ? vk::AttachmentStoreOp::eDontCare : storeOp);

        if (pResolve)
        {
            attachmentInfo
                .setResolveImageLayout(_layout)
                .setResolveImageView(pResolve->getImageView())
                .setResolveMode(isDepth
                    ? vk::ResolveModeFlagBits::eSampleZero
                    : vk::ResolveModeFlagBits::eAverage);

            mResolveImages.push_back(pResolve.get());
        }

        mImages.push_back(pImage.get());

        if (isDepth)
        {
            mDepthAttachmentInfo = attachmentInfo;
        }
        else
        {
            mAttachmentInfos.push_back(attachmentInfo);
        }

        return *this;
    }

    Barrier Rendering::getBarriers()
    {
        auto barriers = Barrier();

        auto both = std::array{ std::span{mImages}, std::span{mResolveImages} };
        for (auto* pImage : both | std::views::join)
        {
            if (vk::hasDepthComponent(pImage->getProperties().format))
            {
                barriers.addBarrier(pImage->getBarrier(ImageUsage::DepthAttachment));
            }
            else
            {
                barriers.addBarrier(pImage->getBarrier(ImageUsage::ColorAttachment));
            }
        }

        return barriers;
    }

    Rendering& Rendering::insertBarriers(const CommandList* pCommandList)
    {
        getBarriers().insert(pCommandList);
        return *this;
    }

    Rendering& Rendering::setViewportScissor(const CommandList* pCommandList)
    {
        const auto scissor  = mRenderArea;
        const auto viewport = vk::Viewport {
            static_cast<float>(mRenderArea.offset.x),
            static_cast<float>(mRenderArea.offset.y),
            static_cast<float>(mRenderArea.extent.width),
            static_cast<float>(mRenderArea.extent.height),
            0.0f,
            1.0f
        };
        pCommandList->setViewportScissor(viewport, scissor);
        return *this;
    }
}

/**
 * Pipelines
 */
namespace RHI
{
    ShaderInfo2 ShaderInfo2::fromFileName(const std::string& fileName)
    {
        using enum vk::ShaderStageFlagBits;
        auto stage = eAll;

        // Resolve shader stage from standard file extensions
        #pragma region
        #define nbl_StageCase(Ext, Stage) if (std::string_view(substr) == Ext) { stage = Stage; }
        for (const auto substr : std::views::split(fileName, '.'))
        {
            nbl_StageCase("vert",   eVertex)
            nbl_StageCase("geom",   eGeometry)
            nbl_StageCase("tc",     eTessellationControl)
            nbl_StageCase("te",     eTessellationEvaluation)
            nbl_StageCase("frag",   eFragment)
            nbl_StageCase("comp",   eCompute)
            nbl_StageCase("mesh",   eMeshEXT)
            nbl_StageCase("task",   eTaskEXT)
            nbl_StageCase("rgen",   eRaygenKHR)
            nbl_StageCase("rchit",  eClosestHitKHR)
            nbl_StageCase("rmiss",  eMissKHR)
            nbl_StageCase("rahit",  eAnyHitKHR)
            nbl_StageCase("rint",   eIntersectionKHR)
            nbl_StageCase("rcall",  eCallableKHR)
        }
        #undef nbl_StageCase
        #pragma endregion

        if (stage == eAll)
        {
            exitWithError("Failed to resolve shader stage from fileName");
        }

        return {
            .path       = Configuration::getShaderFilePath(fileName),
            .stage      = stage,
            .entryPoint = "main"
        };
    }

    #pragma region "PipelineCommon"

    PipelineCommon& PipelineCommon::setLabel(const std::string& value)
    {
        label = value;
        return *this;
    }

    PipelineCommon& PipelineCommon::addShader(const ShaderInfo2& shaderInfo)
    {
        shaders.push_back(shaderInfo);
        return *this;
    }

    PipelineCommon& PipelineCommon::addShader(const std::string& fileName)
    {
        shaders.push_back(ShaderInfo2::fromFileName(fileName));
        return *this;
    }

    PipelineCommon& PipelineCommon::addDescriptorLayout(const uint32_t binding, Descriptor* pDescriptor)
    {
        if (!pDescriptor)
        {
            #ifndef NDEBUG
            exitWithError("The added descriptor must be valid.");
            #endif

            spdlog::warn("The added descriptor must be valid.");
            return *this;
        }

        if (descriptors.contains(binding))
        {
            spdlog::warn("Descriptor binding {} was overwritten. [{} => {}]",
                         binding, descriptors[binding]->getLabel(), pDescriptor->getLabel());
        }

        descriptors[binding] = pDescriptor;
        return *this;
    }

     #pragma endregion

    #pragma region "GraphicsPS"

    GraphicsPS& GraphicsPS::addAttachmentFormat(const vk::Format value)
    {
        const bool isDepth   = vk::hasDepthComponent(value);
        const bool isStencil = vk::hasStencilComponent(value);

        if (isDepth)                depthFormat = value;
        if (isStencil)              stencilFormat = value;
        if (!isDepth && !isStencil) attachmentFormats.push_back(value);

        return *this;
    }

    GraphicsPS& GraphicsPS::addDefaultAttachmentState(const uint32_t count)
    {
        using enum vk::ColorComponentFlagBits;
        for (uint32_t i = 0; i < count; i++)
        {
            constexpr auto state = vk::PipelineColorBlendAttachmentState()
                .setColorWriteMask(eR | eG | eB | eA)
                .setBlendEnable(false)
                .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                .setDstColorBlendFactor(vk::BlendFactor::eZero)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);
            blendStates.push_back(state);
        }
        return *this;
    }

    GraphicsPS& GraphicsPS::addAlphaAttachmentState(const uint32_t count)
    {
        using enum vk::ColorComponentFlagBits;
        for (uint32_t i = 0; i < count; i++)
        {
            constexpr auto state = vk::PipelineColorBlendAttachmentState()
                .setColorWriteMask(eR | eG | eB | eA)
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setAlphaBlendOp(vk::BlendOp::eAdd);
            blendStates.push_back(state);
        }
        return *this;
    }

    GraphicsPS& GraphicsPS::addColorBlendAttachmentState(const vk::PipelineColorBlendAttachmentState& blendState, const uint32_t count)
    {
        for (uint32_t i = 0; i < count; i++)
        {
            blendStates.push_back(blendState);
        }
        return *this;
    }

    GraphicsPS& GraphicsPS::setCullMode(const vk::CullModeFlagBits cullMode)
    {
        rasterizationState.setCullMode(cullMode);
        return *this;
    }

    GraphicsPS& GraphicsPS::setSampleCount(const vk::SampleCountFlagBits samples)
    {
        multisampleState.setRasterizationSamples(samples);
        return *this;
    }

    GraphicsPS& GraphicsPS::setTopology(const vk::PrimitiveTopology topology)
    {
        inputAssemblyState.setTopology(topology);
        return *this;
    }

    GraphicsPS& GraphicsPS::setWireframeMode(const bool value)
    {
        rasterizationState.setPolygonMode(value ? vk::PolygonMode::eFill : vk::PolygonMode::eLine);
        return *this;
    }

    vk::PipelineInputAssemblyStateCreateInfo GraphicsPS::makeInputAssemblyState()
    {
        return vk::PipelineInputAssemblyStateCreateInfo()
            .setTopology(vk::PrimitiveTopology::eTriangleList)
            .setPrimitiveRestartEnable(false)
            .setFlags({})
            .setPNext(nullptr);
    }

    vk::PipelineRasterizationStateCreateInfo GraphicsPS::makeRasterizationState()
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

    vk::PipelineMultisampleStateCreateInfo GraphicsPS::makeMultisampleState()
    {
        return vk::PipelineMultisampleStateCreateInfo()
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setSampleShadingEnable(false)
            .setPSampleMask(nullptr)
            .setAlphaToCoverageEnable(false)
            .setAlphaToOneEnable(false)
            .setPNext(nullptr);
    }

    vk::PipelineDepthStencilStateCreateInfo GraphicsPS::makeDepthStencilState()
    {
        return vk::PipelineDepthStencilStateCreateInfo()
            .setDepthTestEnable(true)
            .setDepthWriteEnable(true)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setDepthBoundsTestEnable(false)
            .setStencilTestEnable(false);
    }

    vk::PipelineViewportStateCreateInfo GraphicsPS::makeViewportState()
    {
        return vk::PipelineViewportStateCreateInfo()
            .setViewportCount(1)
            .setPViewports(nullptr)
            .setScissorCount(1)
            .setPScissors(nullptr)
            .setPNext(nullptr);
    }

    vk::PipelineDynamicStateCreateInfo GraphicsPS::makeDynamicState()
    {
        return vk::PipelineDynamicStateCreateInfo()
            .setDynamicStateCount(0)
            .setPDynamicStates(nullptr)
            .setPNext(nullptr);
    }

    vk::PipelineColorBlendStateCreateInfo GraphicsPS::makeColorBlendState()
    {
        return vk::PipelineColorBlendStateCreateInfo()
            .setLogicOp(vk::LogicOp::eClear)
            .setLogicOpEnable(false)
            .setAttachmentCount(0)
            .setPAttachments(nullptr)
            .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f})
            .setPNext(nullptr);
    }

    vk::PipelineVertexInputStateCreateInfo GraphicsPS::makeVertexInputState()
    {
        return vk::PipelineVertexInputStateCreateInfo()
            .setVertexAttributeDescriptionCount(0)
            .setPVertexAttributeDescriptions(nullptr)
            .setVertexBindingDescriptionCount(0)
            .setPVertexBindingDescriptions(nullptr)
            .setPNext(nullptr);
    }

    #pragma endregion

    #pragma region "RayTracingPS"

    RayTracingPS& RayTracingPS::setMaxDepth(const uint32_t value)
    {
        maxDepth = value;
        return *this;
    }

    #pragma endregion

    #pragma region "Pipeline"

    PipelineBase::PipelineBase(const PipelineCommon& common, const PipelineType2 type, const SPtr<Device>& device)
    : Resource(device)
    , mType(type)
    , mBindPoint(toBindPoint(type))
    , mDescriptors(common.descriptors)
    , mPushConstantRange(common.pushConstantRange)
    {
        setLabel(common.label);
        spdlog::debug("Creating Pipeline: {} [type={}]", styled(common.label, fg(fmt::color::cyan)), toString(type));

        createPipelineLayout();
    }

    PipelineBase::~PipelineBase()
    {
        mDevice->waitIdle(); // TODO: Test if this is even required
        mDevice->getHandle().destroy(mPipeline);
        mDevice->getHandle().destroy(mPipelineLayout);

        spdlog::debug("Destroyed Pipeline: {} [type={}]", styled(mLabel, fg(fmt::color::pale_violet_red)), toString(mType));
    }

    const vk::Pipeline& PipelineBase::getHandle() const noexcept
    {
        return mPipeline;
    }

    const vk::PipelineLayout& PipelineBase::getLayout() const noexcept
    {
        return mPipelineLayout;
    }

    PipelineType2 PipelineBase::getType() const noexcept
    {
        return mType;
    }

    const vk::PipelineBindPoint& PipelineBase::getBindPoint() const noexcept
    {
        return mBindPoint;
    }

    const std::optional<vk::PushConstantRange>& PipelineBase::getPushConstantRange() const noexcept
    {
        return mPushConstantRange;
    }

    vk::PipelineBindPoint PipelineBase::toBindPoint(const PipelineType2 type)
    {
        using enum PipelineType2;
        switch (type)
        {
            case Compute:    return vk::PipelineBindPoint::eCompute;
            case RayTracing: return vk::PipelineBindPoint::eRayTracingKHR;
            default:         return vk::PipelineBindPoint::eGraphics;
        }
    }

    std::vector<char> PipelineBase::readShaderFile(const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        exitOnAssert(file.is_open(), "Failed to open file: {}", filePath.string().c_str());

        const std::streamsize fileSize = file.tellg();
        std::vector<char>     buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    PipelineBase::LoadedShader PipelineBase::loadShader(const ShaderInfo2& shaderInfo)
    {
        LoadedShader result = {};
        result.shaderInfo   = shaderInfo;
        result.code         = readShaderFile(shaderInfo.path);
        return result;
    }

    void PipelineBase::createPipelineLayout()
    {
        std::vector<vk::DescriptorSetLayout> layouts;
        if (!mDescriptors.empty())
        {
            std::vector<uint32_t> keys;
            for (const auto& key : mDescriptors | std::views::keys)
            {
                keys.push_back(key);
            }
            std::ranges::sort(keys);

            layouts.resize(keys.back() + 1, VK_NULL_HANDLE);
            for (const auto setIndex : keys)
            {
                layouts[setIndex] = mDescriptors[setIndex]->getLayout();
            }
        }

        auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayouts(layouts);

        if (mPushConstantRange.has_value())
        {
            layoutCreateInfo.setPushConstantRanges(mPushConstantRange.value());
        }

        try {
            mPipelineLayout = mDevice->getHandle().createPipelineLayout(layoutCreateInfo);

            mDevice->nameObject<vk::PipelineLayout>({
                .debugName = fmt::format("{}_Layout", mLabel),
                .handle    = mPipelineLayout,
            });
        } catch (const vk::SystemError& err) {
            exitWithError("Failed to create PipelineLayout for pipeline [{}]: {}", mLabel, err.what());
        }
    }

    #pragma endregion
}
