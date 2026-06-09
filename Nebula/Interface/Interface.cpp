#include "Interface.hpp"

#include "Level/Render/Templates.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    Interface::Interface(const InterfaceParams& params)
    : mRHI(params.rhi)
    , mTextureManager(params.pTextureManager)
    , mSize(params.size, params.padding)
    {
        mFontLibrary = makeUnique<FontLibrary>(mTextureManager);
        for (const auto& fontSource : params.fonts)
        {
            mFontLibrary->registerSource(fontSource);
        }

        mFontLibrary->registerSource({
            .file = Configuration::getFontFilePath("ExodusDemo-Striped.otf"),
            .name = "Exodus",
        });
        auto text = makeUnique<TextElement>("FORSAKEN", glm::vec3(1.0f), mFontLibrary->getFont("Exodus", 128));
        text->mPosition = mSize.getPosition(Location::Center, text.get());

        mElements.push_back(std::move(text));

        uint32_t              vtxBase = 0;
        std::vector<UIVertex> vertices;
        std::vector<uint32_t> indices;
        for (const auto& elem : mElements)
        {
            const auto g = elem->getGeometry();

            const auto info = ElemGeometryInfo {
                .firstIndex  = static_cast<uint32_t>(indices.size()),
                .indexCount  = static_cast<uint32_t>(g.indices.size()),
                .firstVertex = vtxBase,
                .vertexCount = static_cast<uint32_t>(g.vertices.size()),
            };

            const auto offsetIndices = g.indices
                | std::views::transform([vtxBase](const uint32_t i){ return i + vtxBase; });

            indices.append_range(offsetIndices);
            vertices.append_range(g.vertices);

            vtxBase += g.vertices.size();

            mGeometryInfo.push_back(info);
        }

        /* Vertex & Index Buffer */
        {
            mVertexBuffer = mRHI->createBuffer({
                .size  = vertices.size() * sizeof(UIVertex),
                .type  = RHI::BufferType::Vertex,
                .label = "UIVertices",
            });
            mRHI->immediate_uploadToBuffer(mVertexBuffer.get(), vertices.data(), vertices.size() * sizeof(UIVertex), 0);
            mIndexBuffer = mRHI->createBuffer({
                .size  = indices.size() * sizeof(uint32_t),
                .type  = RHI::BufferType::Index,
                .label = "UIIndices",
            });
            mRHI->immediate_uploadToBuffer(mIndexBuffer.get(), indices.data(), indices.size() * sizeof(uint32_t), 0);
        }

        /* Render Targets */
        {
            using enum vk::ImageUsageFlagBits;
            for (auto i = 0; i < mResult.size(); i++)
            {
                mResult[i] = mRHI->createImage({
                    .extent        = { static_cast<uint32_t>(mSize.size.x), static_cast<uint32_t>(mSize.size.y) },
                    .format        = vk::Format::eR16G16B16A16Sfloat,
                    .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
                    .samples       = vk::SampleCountFlagBits::e4,
                    .createSampler = true,
                    .debugName     = fmt::format("UI_Result_{}", i),
                });

                mResolvedResult[i] = mRHI->createImage({
                    .extent        = { static_cast<uint32_t>(mSize.size.x), static_cast<uint32_t>(mSize.size.y) },
                    .format        = vk::Format::eR16G16B16A16Sfloat,
                    .usageFlags    = eColorAttachment | eSampled | eTransferSrc | eTransferDst,
                    .createSampler = true,
                    .debugName     = fmt::format("UI_Result_Resolved_{}", i),
                });

                mRenderPass[i] = mRHI->createRenderPass({
                    .renderArea       = mSize.getRenderArea(),
                    .colorAttachments = { makeResolvedAttachment(mResult[i], mResolvedResult[i]) },
                    .label            = fmt::format("UI_RenderPass_{}", i),
                });
            }
        }

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mTextureManager->getDescriptor()->getLayout())
            .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant) })
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](RHI::GraphicsPipelineStateInfo& stateInfo) {
                    stateInfo.addAttributeDescriptions<UIVertex>(0, 0);
                    stateInfo.addBindingDescriptions<UIVertex>(0);
                    stateInfo.multisampleState.setRasterizationSamples(vk::SampleCountFlagBits::e4);
                })
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addAttachmentState(RHI::PipelineUtils::makeColorBlendAttachmentState()
                    .setBlendEnable(true)
                        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                        .setColorBlendOp(vk::BlendOp::eAdd)
                        .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                        // .setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                        .setAlphaBlendOp(vk::BlendOp::eAdd)))
            .addShader({ Configuration::getShaderFilePath("UI.vert.spv"), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("UI.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mResult[0]->getProperties().format)
            .setDebugName("UI_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    void Interface::render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        const auto i = frameData.currentFrame;

        pCommandList->beginLabel("UI Test");

        RHI::Barrier()
            .addBarrier(mResult[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mResolvedResult[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        mRenderPass[i]->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            const auto [w, h, d] = mResult[i]->getProperties().getExtent3D();
            const auto scissor   = vk::Rect2D {{ 0, 0 }, {w, h} };
            const auto viewport  = vk::Viewport { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
            pCommandList->setViewportScissor(viewport, scissor);

            PushConstant pushConstant;
            pushConstant.proj = mSize.projection;

            mPipeline->bind(cmd);
            mPipeline->bindDescriptorSet(cmd, mTextureManager->getDescriptor()->getSet(0));

            constexpr vk::DeviceSize offset = 0;
            cmd->getHandle().bindVertexBuffers(0, 1, &mVertexBuffer->getHandle(), &offset);
            cmd->getHandle().bindIndexBuffer(mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);

            for (auto j = 0; j < mElements.size(); j++)
            {
                pushConstant.isText = 1;
                pushConstant.textureIndex = static_cast<int32_t>(mElements[j]->getTextureIndex());
                pushConstant.color = glm::vec4(1.0f);
                mPipeline->pushConstants(cmd, &pushConstant);
                pCommandList->getHandle().drawIndexed(mGeometryInfo[j].indexCount, 1, mGeometryInfo[j].firstIndex, 0, 0);
            }
        });
        pCommandList->endLabel();
    }
}