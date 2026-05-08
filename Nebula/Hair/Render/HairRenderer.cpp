#include "HairRenderer.hpp"

#include "Core/Random.hpp"

namespace nbl
{
    HairRenderer::HairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModels)
    : mRHI(rhi)
    , mHairModels(pHairModels)
    {
        createDebugColors();
        createResources();
        createPipeline();
    }

    void HairRenderer::render(
        const RHI::CommandList* pCommandList,
        const RHI::FrameData&   frameData,
        const uint32_t          hairIndex,
        const uint64_t          cameraBuffer)
    {
        pCommandList->beginLabel("Hair");

        const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, -45.0f)).getModel();

        RHI::Barrier()
            .addBarrier(mRenderTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mDepthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(mDebugColorsBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
            .insert(pCommandList);

        mRenderPass[frameData.currentFrame]->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
        {
            const auto& info = mHairModels->mHairInfos[hairIndex];
            const auto pushConstants = PushConstants
            {
                .model                   = model,
                .diffuse                 = glm::vec4(0.32549f, 0.23921f, 0.20784f, 1.0f),
                .specular                = glm::vec4(0.41568f, 0.30588f, 0.21960f, 1.0f),
                .vertexBufferAddress     = mHairModels->mHairVertices->getAddress(),
                .attributesBufferAddress = mHairModels->mHairAttributes->getAddress(),
                .strandDescBufferAddress = mHairModels->mStrandDescriptions->getAddress(),
                .debugColorBufferAddress = mDebugColorsBuffer->getAddress(),
                .cameraBufferAddress     = cameraBuffer,
                .firstVertex             = info.firstVertex,
                .vertexCount             = info.vertexCount,
                .firstStrand             = info.firstStrand,
                .strandCount             = info.strandCount,
                .renderMode              = std::to_underlying(mRenderingMode),
                ._pad0                   = 0,
            };

            mPipeline->bind(cmd);
            mPipeline->pushConstants(cmd, &pushConstants);

            const auto taskGroupSizeX = static_cast<uint32_t>(std::floor(info.strandCount / gHairMaxStrandletSize));
            cmd->getHandle().drawMeshTasksEXT(taskGroupSizeX, 1, 1);
        });

        pCommandList->endLabel();
    }

    void HairRenderer::createDebugColors()
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

    void HairRenderer::createResources()
    {
        for (size_t i = 0; i < mRenderTarget.size(); i++)
        {
            mRenderTarget[i] = makeRenderTarget(mRHI.get(), fmt::format("HairRenderer_Target_{}", i));
            mDepthBuffer[i]  = makeRenderTarget(mRHI.get(), fmt::format("HairRenderer_Depth_{}", i), vk::Format::eD32Sfloat);
        }
    }

    void HairRenderer::createPipeline()
    {
        mScissor = getRenderAreaForAttachment(mRenderTarget[0].get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        for (size_t i = 0; i < mRenderPass.size(); i++)
        {
            mRenderPass[i] = mRHI->createRenderPass({
                .renderArea       = mScissor,
                .colorAttachments = { makeAttachment(mRenderTarget[i]) },
                .depthAttachment  = makeAttachment(mDepthBuffer[i]),
                .label            = fmt::format("Hair_RenderPass_", i),
            });
        }

        const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eFragment)
            .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
            {
                stateInfo
                    .addDefaultAttachmentStates(1)
                    .setCullMode(vk::CullModeFlagBits::eNone);
            }))
            .addShader({ Configuration::getShaderFilePath("hair.task.spv"), vk::ShaderStageFlagBits::eTaskEXT })
            .addShader({ Configuration::getShaderFilePath("hair.mesh.spv"), vk::ShaderStageFlagBits::eMeshEXT })
            .addShader({ Configuration::getShaderFilePath("hair.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRenderTarget[0]->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer[0]->getProperties().format)
            .setDebugName("Hair_Classic_Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}
