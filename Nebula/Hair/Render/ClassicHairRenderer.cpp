#include "ClassicHairRenderer.hpp"

namespace nbl
{
    ClassicHairRenderer::ClassicHairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModels)
    : mRHI(rhi)
    , mHairModels(pHairModels)
    {
        createDebugColors();
        createResources();
        createPipeline();
    }

    void ClassicHairRenderer::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const HairRenderer_BDAs& buffers)
    {
        pCommandList->beginLabel("Hair_Classic");

        const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, -45.0f)).getModel();

        RHI::Barrier()
            .addBarrier(mRenderTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mDepthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(mDebugColorsBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
            .insert(pCommandList);

        RHI::Rendering()
            .setRenderArea(mScissor)
            .addAttachment(mRenderTarget[frameData.currentFrame])
            .addAttachment(mDepthBuffer[frameData.currentFrame])
            .setLabel(fmt::format("Hair_Classic_RenderPass"))
            .execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
            {
                const auto& info          = mHairModels->mHairInfos[0];
                const auto  pushConstants = PushConstants
                {
                    .model                   = model,
                    .diffuse                 = glm::vec4(0.32549f, 0.23921f, 0.20784f, 1.0f),
                    .specular                = glm::vec4(0.41568f, 0.30588f, 0.21960f, 1.0f),
                    .vertexBufferAddress     = mHairModels->mHairVertices->getAddress(),
                    .strandDescBufferAddress = mHairModels->mStrandDescriptions->getAddress(),
                    .debugColorBufferAddress = mDebugColorsBuffer->getAddress(),
                    .cameraBufferAddress     = buffers.cameraBuffer,
                    .firstVertex             = info.firstVertex,
                    .vertexCount             = info.vertexCount,
                    .firstStrand             = info.firstStrand,
                    .strandCount             = info.strandCount,
                };

                mPipeline->bind(cmd);
                mPipeline->pushConstants(cmd, &pushConstants);

                const auto taskGroupSizeX = static_cast<uint32_t>(std::floor(info.strandCount / gHairMaxStrandletSize));
                cmd->getHandle().drawMeshTasksEXT(taskGroupSizeX, 1, 1);
            });

        pCommandList->endLabel();
    }

    const SPtr<RHI::Image>& ClassicHairRenderer::getResult(const uint32_t frameIndex) const
    {
        return mRenderTarget[frameIndex];
    }

    void ClassicHairRenderer::createDebugColors()
    {
        for (size_t i = 0; i < mDebugColors.size(); i++)
        {
            mDebugColors[i] = Random::getColor();
        }
        mDebugColorsBuffer = mRHI->createBuffer({
            .size  = mDebugColors.size() * sizeof(glm::vec4),
            .type  = RHI::BufferType::Storage,
            .label = "HairRenderer_DebugColorsBuffer",
        });
        mRHI->immediate_uploadToBuffer(mDebugColorsBuffer.get(), mDebugColors.data(), mDebugColors.size() * sizeof(glm::vec4));
    }

    void ClassicHairRenderer::createResources()
    {
        for (size_t i = 0; i < mRenderTarget.size(); i++)
        {
            mRenderTarget[i] = makeRenderTarget(mRHI.get(), fmt::format("HairRenderer_Target_{}", i));
            mDepthBuffer[i]  = makeRenderTarget(mRHI.get(), fmt::format("HairRenderer_Depth_{}", i), vk::Format::eD32Sfloat);
        }
    }

    void ClassicHairRenderer::createPipeline()
    {
        mScissor  = getRenderAreaForAttachment(mRenderTarget[0].get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eFragment)
            .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
            {
                stateInfo
                    .addDefaultAttachmentStates(1)
                    .setCullMode(vk::CullModeFlagBits::eNone);
            }))
            .addShader({ Configuration::getShaderFilePath("Hair_Classic.task.spv"), vk::ShaderStageFlagBits::eTaskEXT })
            .addShader({ Configuration::getShaderFilePath("Hair_Classic.mesh.spv"), vk::ShaderStageFlagBits::eMeshEXT })
            .addShader({ Configuration::getShaderFilePath("Hair_Classic.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRenderTarget[0]->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer[0]->getProperties().format)
            .setDebugName("Hair_Classic_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}
