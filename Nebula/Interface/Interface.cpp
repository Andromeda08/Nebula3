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
        mFontLibrary->registerSource({
            .file = Configuration::getFontFilePath("PPFraktionMono-Regular.otf"),
            .name = "Fraktion",
        });
        mFontLibrary->registerSource({
            .file = Configuration::getFontFilePath("KHInterference-Regular.otf"),
            .name = "Interference",
        });

        {
            auto text = makeUnique<TextElement>("Nebula", glm::vec3(1.0f), mFontLibrary->getFont("Fraktion", 64));
            text->mPosition = mSize.getPosition(Location::CenterLeft, text.get()) + glm::vec2(64.0f, -32.0f);
            mElements.push_back(std::move(text));
        }
        {
            auto text = makeUnique<TextElement>("press [space] to play", glm::vec3(1.0f), mFontLibrary->getFont("Fraktion", 24));
            text->mPosition = mSize.getPosition(Location::CenterLeft, text.get()) + glm::vec2(64.0f, 32.0f);
            text->mColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

            mElements.push_back(std::move(text));
        }

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
            }
        }

        using enum vk::ShaderStageFlagBits;
        const auto graphicsPS = RHI::GraphicsPS()
            .addVertexType<UIVertex>()
            .configure([](auto& s){ s.multisampleState.setRasterizationSamples(vk::SampleCountFlagBits::e4); })
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addAlphaAttachmentState(1)
            .addAttachmentFormat(mResult[0]->getProperties().format);
        const auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("UI_Pipeline")
            .addShader("UI.vert.spv")
            .addShader("UI.frag.spv")
            .setPushConstant<PushConstant>(eVertex | eFragment)
            .addDescriptorLayout(0, mTextureManager->getDescriptor().get());

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    void Interface::render(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        const auto i = frameData.currentFrame;

        pCommandList->beginLabel("UI Test");

        RHI::Barrier()
            .addBarrier(mResult[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mResolvedResult[i]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .insert(pCommandList);

        RHI::Rendering()
            .addAttachment(mResult[i], vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, std::nullopt, mResolvedResult[i])
            .setRenderArea(mSize.getRenderArea())
            .setLabel(fmt::format("UI_RenderPass_{}", i))
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void {
                const auto [w, h, d] = mResult[i]->getProperties().getExtent3D();
                const auto scissor   = vk::Rect2D {{ 0, 0 }, {w, h} };
                const auto viewport  = vk::Viewport { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
                pCommandList->setViewportScissor(viewport, scissor);

                PushConstant pushConstant;
                pushConstant.proj = mSize.projection;

                cmd->bindPipeline(mPipeline.get());
                cmd->bindDescriptorSet(mTextureManager->getDescriptor()->getSet(), 0);

                constexpr vk::DeviceSize offset = 0;
                cmd->getHandle().bindVertexBuffers(0, 1, &mVertexBuffer->getHandle(), &offset);
                cmd->getHandle().bindIndexBuffer(mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);

                for (auto j = 0; j < mElements.size(); j++)
                {
                    pushConstant.isText = 1;
                    pushConstant.textureIndex = static_cast<int32_t>(mElements[j]->getTextureIndex());
                    pushConstant.color = glm::vec4(1.0f);
                    cmd->pushConstants(&pushConstant);
                    cmd->getHandle().drawIndexed(mGeometryInfo[j].indexCount, 1, mGeometryInfo[j].firstIndex, 0, 0);
                }
            });

        pCommandList->endLabel();
    }
}