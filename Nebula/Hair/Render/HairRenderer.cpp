#include "HairRenderer.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

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
        const uint32_t          _hairIndex,
        const uint64_t          cameraBuffer)
    {
        pCommandList->beginLabel("Hair");

        const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, -45.0f)).getModel();

        RHI::Barrier()
            .addBarrier(mRenderTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mDepthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
            .addBarrier(mDebugColorsBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::StorageRead))
            .insert(pCommandList);

        pCommandList->setViewportScissor(mViewport, mScissor);

        mRenderPass[frameData.currentFrame]->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
        {
            const auto& info = mHairModels->mHairInfos[mHairIndex];
            const auto pushConstants = PushConstants
            {
                .model                   = model,
                .diffuse                 = mDiffuse,
                .specular                = mSpecular,
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
                .useCustomColors         = mUseCustomColor ? 1 : 0,
                .specularFactor          = mSpecularFactor,
                ._pad0                   = 0,
            };

            mPipeline->bind(cmd);
            mPipeline->pushConstants(cmd, &pushConstants);

            auto taskGroupSizeX = mHairModels->getHairGeometry(static_cast<size_t>(mHairIndex)).taskGroupSizeX;
            if (mUseCustomWgSize)
            {
                taskGroupSizeX = static_cast<uint32_t>(mCustomTaskWgSize);
            }
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

    void HairRendererUI::draw()
    {
        ImGui::Begin("Hair Renderer Config");

        ImGui::SliderInt("Hair Model", &mHairRenderer->mHairIndex, 0, mHairRenderer->mHairModels->getModelCount() - 1);

        ImGui::DragFloat("Specular", &mHairRenderer->mSpecularFactor, 0.05f, 0.0f, 32.0f);

        ImGui::SeparatorText("Task Workgroup Size");
        const auto defaultGroupSize = mHairRenderer->mHairModels->getHairGeometry(mHairRenderer->mHairIndex).taskGroupSizeX;
        ImGui::Text("Default size: %u", defaultGroupSize);
        ImGui::Checkbox("Override", &mHairRenderer->mUseCustomWgSize);
        ImGui::SliderInt("X", &mHairRenderer->mCustomTaskWgSize, 0, defaultGroupSize);

        ImGui::SeparatorText("Custom Color");
        ImGui::Checkbox("Enable", &mHairRenderer->mUseCustomColor);
        ImGui::ColorEdit4("Diffuse", glm::value_ptr(mHairRenderer->mDiffuse));
        ImGui::ColorEdit4("Specular", glm::value_ptr(mHairRenderer->mSpecular));

        ImGui::End();
    }
}
