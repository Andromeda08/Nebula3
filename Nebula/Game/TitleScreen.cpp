#include "TitleScreen.hpp"

#include "VulkanRHI/Barrier.hpp"

TitleScreen::TitleScreen(const glm::vec2& size, const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager)
: mScreen(size)
, mRHI(rhi)
, mTextureManager(pTextureManager)
{
    mFont48 = makeUnique<FontFace>(mTextureManager, Configuration::getFontFilePath("KHInterference-Regular.otf"), 32.0f);
    mFont16 = makeUnique<FontFace>(mTextureManager, Configuration::getFontFilePath("PPFraktionMono-Regular.otf"), 16.0f);

    mQuads.push_back({
        .center          = glm::vec2(mScreen.size.x - mScreen.padding.x - 24.0f, mScreen.size.y - mScreen.padding.y - 24.0f),
        .sideLengths     = { 16.0f, 16.0f },
        .backgroundColor = glm::vec4(0.32f, 0.0f, 1.0f, 1.0f),
        .textureIndex    = -1
    });

    mQuads.push_back({
        .center          = glm::vec2(mScreen.size.x - mScreen.padding.x - 24.0f, mScreen.padding.y + 24.0f),
        .sideLengths     = { 16.0f, 16.0f },
        .backgroundColor = glm::vec4(0.32f, 0.0f, 1.0f, 1.0f),
        .textureIndex    = -1
    });

    mQuads.push_back({
        .center          = glm::vec2(mScreen.padding.x + 24.0f, mScreen.padding.y + 24.0f),
        .sideLengths     = { 16.0f, 16.0f },
        .backgroundColor = glm::vec4(0.32f, 0.0f, 1.0f, 1.0f),
        .textureIndex    = -1
    });

    mQuads.push_back({
        .center          = mScreen.getAnchorPosition(Anchor::TopCenter, glm::vec2(128.0f, 32.0f)),
        .sideLengths     = { 128.0f, 32.0f },
        .backgroundColor = glm::vec4(0.76078f, 0.99608f, 0.04314f, 1.0f),
        .textureIndex    = -1,
    });

    mQuads.push_back({
        .center          = mScreen.getAnchorPosition(Anchor::BottomCenter, glm::vec2(128.0f, 32.0f)),
        .sideLengths     = { 128.0f, 32.0f },
        .backgroundColor = glm::vec4(0.76078f, 0.99608f, 0.04314f, 1.0f),
        .textureIndex    = -1,
    });

    mQuads.push_back({
        .center          = mScreen.getAnchorPosition(Anchor::MiddleLeft, glm::vec2(32.0f, 128.0f)),
        .sideLengths     = { 32.0f, 128.0f },
        .backgroundColor = glm::vec4(0.76078f, 0.99608f, 0.04314f, 1.0f),
        .textureIndex    = -1,
    });

    mQuads.push_back({
        .center          = mScreen.getAnchorPosition(Anchor::MiddleRight, glm::vec2(32.0f, 128.0f)),
        .sideLengths     = { 32.0f, 128.0f },
        .backgroundColor = glm::vec4(0.76078f, 0.99608f, 0.04314f, 1.0f),
        .textureIndex    = -1,
    });

    uint32_t              base = 0;
    std::vector<UIVertex> vertices;
    std::vector<uint32_t> indices;
    for (const auto& quad : mQuads)
    {
        const auto geom = quad.toUIGeometry();

        const auto offsetIndices = geom.indices
            | std::views::transform([base](const uint32_t i){ return i + base; });

        indices.append_range(offsetIndices);
        vertices.append_range(geom.vertices);

        base += geom.vertices.size();
    }

    mText.push_back({
        .text     = "Nebula 3",
        .position = glm::vec2(mScreen.padding.x, mScreen.size.y - mScreen.padding.y - 38.0f),
        .color    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        .pFont    = mFont48.get(),
    });

    mText.push_back({
        .text     = "/ GI and Systems Preview /",
        .position = glm::vec2(mScreen.padding.x, mScreen.size.y - mScreen.padding.y - 16.0f),
        .color    = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
        .pFont    = mFont16.get(),
    });

    for (auto& txt : mText)
    {
        const auto g = txt.pFont->buildTextGeometry(txt.text, txt.position);

        txt.indexOffset = static_cast<uint32_t>(indices.size());
        txt.indexCount  = static_cast<uint32_t>(g.indices.size());

        const auto offsetIndices = g.indices
            | std::views::transform([base](const uint32_t i){ return i + base; });

        indices.append_range(offsetIndices);
        vertices.append_range(g.vertices);

        base += g.vertices.size();
    }

    mVertices = mRHI->createBuffer({
        .size  = vertices.size() * sizeof(UIVertex),
        .type  = RHI::BufferType::Vertex,
        .label = "UIVertices",
    });
    mRHI->immediate_uploadToBuffer(mVertices.get(), vertices.data(), vertices.size() * sizeof(UIVertex), 0);
    mIndices = mRHI->createBuffer({
        .size  = indices.size() * sizeof(uint32_t),
        .type  = RHI::BufferType::Index,
        .label = "UIIndices",
    });
    mRHI->immediate_uploadToBuffer(mIndices.get(), indices.data(), indices.size() * sizeof(uint32_t), 0);

    using enum vk::ImageUsageFlagBits;
    mResult = mRHI->createImage({
        .extent        = { static_cast<uint32_t>(mScreen.size.x), static_cast<uint32_t>(mScreen.size.y) },
        .format        = vk::Format::eR16G16B16A16Sfloat,
        .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
        .createSampler = true,
        .debugName     = "UI_Result",
    });

    mRenderPass = mRHI->createRenderPass({
        .renderArea = {{0, 0}, {static_cast<uint32_t>(mScreen.size.x), static_cast<uint32_t>(mScreen.size.y)}},
        .colorAttachments = {
            RHI::Attachment {
                .image          = mResult->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mResult->getImageView())
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
            }
        },
        .label = "UI_RenderPass",
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .addDescriptorSetLayout(mTextureManager->getDescriptor()->getLayout())
        .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant) })
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                stateInfo.addAttributeDescriptions<UIVertex>(0, 0);
                stateInfo.addBindingDescriptions<UIVertex>(0);
            })
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addAttachmentState(RHI::PipelineUtils::makeColorBlendAttachmentState()
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setAlphaBlendOp(vk::BlendOp::eAdd)))
        .addShader({ Configuration::getShaderFilePath("UI.vert.spv"), vk::ShaderStageFlagBits::eVertex })
        .addShader({ Configuration::getShaderFilePath("UI.frag.spv"), vk::ShaderStageFlagBits::eFragment })
        .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
        .setDebugName("UI_Pipeline");

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}

void TitleScreen::render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
{
    pCommandList->beginLabel("UI Test");
    mRenderPass->setColorAttachment(0, RHI::Attachment {
        .image          = mRHI->getSwapchain()->getImage(frameData.acquiredIndex),
        .attachmentInfo = vk::RenderingAttachmentInfo()
            .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(mRHI->getSwapchain()->getImageView(frameData.acquiredIndex))
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
    });

    RHI::Barrier()
        .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::ColorAttachment))
        .insert(pCommandList);

    mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
        const auto [w, h, d] = mResult->getProperties().getExtent3D();
        const auto scissor   = vk::Rect2D {{ 0, 0 }, {w, h} };
        const auto viewport  = vk::Viewport { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
        pCommandList->setViewportScissor(viewport, scissor);

        PushConstant pushConstant;
        pushConstant.proj = mScreen.proj;

        mPipeline->bind(cmd);
        mPipeline->bindDescriptorSet(cmd, mTextureManager->getDescriptor()->getSet(0));

        constexpr vk::DeviceSize offset = 0;
        cmd->getHandle().bindVertexBuffers(0, 1, &mVertices->getHandle(), &offset);
        cmd->getHandle().bindIndexBuffer(mIndices->getHandle(), 0, vk::IndexType::eUint32);
        // for (auto i = 0; i < mQuads.size(); i++)
        // {
        //     pushConstant.color = mQuads[i].backgroundColor;
        //     mPipeline->pushConstants(cmd, &pushConstant);
        //     pCommandList->getHandle().drawIndexed(6, 1, i * 6, 0, 0);
        // }

        pushConstant.isText       = 1;
        for (auto i = 0; i < mText.size(); i++)
        {
            pushConstant.textureIndex = static_cast<int32_t>(mText[i].pFont->getTextureIndex());
            pushConstant.color = mText[i].color;
            mPipeline->pushConstants(cmd, &pushConstant);
            pCommandList->getHandle().drawIndexed(mText[i].indexCount, 1, mText[i].indexOffset, 0, 0);
        }
    });
    pCommandList->endLabel();
}
