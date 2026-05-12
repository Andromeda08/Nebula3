#pragma once

#include "Shared.hpp"
#include "Level/Transform.hpp"
#include "Level/Render/Templates.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HybridHair_MeshStage
    {
        struct PushConstants
        {
            glm::mat4 model;
            glm::vec4 diffuse;
            glm::vec4 specular;

            // Buffer References
            uint64_t  vertexBuffer;
            uint64_t  attributesBuffer;
            uint64_t  strandDescriptionBuffer;
            uint64_t  cameraBuffer;
            uint64_t  trianglesBuffer;
            uint64_t  colorsBuffer;

            // Hair model specific global buffer offsets
            uint32_t  firstVertex;
            uint32_t  vertexCount;
            uint32_t  firstStrand;
            uint32_t  strandCount;

            // Path choosing
            uint64_t  smallTriangleCounterBuffer;
            uint32_t  maxSmallTriangles;
            float     smallTriangleThreshold;
            float     width;
            float     height;

            // Extras
            float     specularFactor;
            int32_t   useCustomColor;
        };

    public:
        HybridHair_MeshStage(const SPtr<RHI::VulkanRHI>& rhi, HairShared* pShared)
        : mRHI(rhi)
        , mShared(pShared)
        {
            createResources();
            createPipeline();
        }

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const uint64_t cameraBufferAddress) const
        {
            pCommandList->beginLabel("MeshStage");

            const auto& counterBuffer = mShared->smallTriangleCounterBuffer[frameData.currentFrame];

            // Clear counter buffer
            {
                pCommandList->beginLabel("Preamble");
                RHI::Barrier()
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::TransferDst))
                    .insert(pCommandList);
                pCommandList->getHandle().fillBuffer(counterBuffer->getHandle(), 0, sizeof(uint32_t), 0);
                pCommandList->endLabel();
            }

            // Dispatch mesh pipeline
            {
                const auto& trianglesBuffer = mShared->trianglesBuffer[frameData.currentFrame];

                pCommandList->beginLabel("Execution");
                RHI::Barrier()
                    .addBarrier(mRenderTarget[frameData.currentFrame]->getBarrier(RHI::ImageUsage::ColorAttachment))
                    .addBarrier(mDepthBuffer[frameData.currentFrame]->getBarrier(RHI::ImageUsage::DepthAttachment))
                    .addBarrier(counterBuffer->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::Compute_Write))
                    .addBarrier(trianglesBuffer->getBarrier(RHI::BufferUsage::All, RHI::BufferUsage::Compute_Write))
                    .insert(pCommandList);

                pCommandList->setViewportScissor(mViewport, mScissor);

                mRenderPass[frameData.currentFrame]->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void
                {
                    const glm::mat4 model = Transform().setRotation(glm::vec3(-90.0f, 0.0f, -45.0f)).getModel();
                    const auto& info = mShared->hairModels->getHairInfo(mShared->config.hairIndex);

                    const auto pushConstants = PushConstants
                    {
                        .model                      = model,
                        .diffuse                    = glm::vec4(mShared->config.diffuse, 1.0f),
                        .specular                   = glm::vec4(mShared->config.specular, 1.0f),
                        .vertexBuffer               = mShared->hairModels->getVertexAddress(),
                        .attributesBuffer           = mShared->hairModels->getAttributesAddress(),
                        .strandDescriptionBuffer    = mShared->hairModels->getStrandDescriptionsAddress(),
                        .cameraBuffer               = cameraBufferAddress,
                        .trianglesBuffer            = trianglesBuffer->getAddress(),
                        .colorsBuffer               = mShared->colorsBuffer->getAddress(),
                        .firstVertex                = info.firstVertex,
                        .vertexCount                = info.vertexCount,
                        .firstStrand                = info.firstStrand,
                        .strandCount                = info.strandCount,
                        .smallTriangleCounterBuffer = counterBuffer->getAddress(),
                        .maxSmallTriangles          = info.vertexCount * 2,
                        .smallTriangleThreshold     = mShared->config.smallTriangleThreshold,
                        .width                      = mViewport.width,
                        .height                     = mViewport.height,
                        .specularFactor             = mShared->config.specularFactor,
                        .useCustomColor             = mShared->config.overrideColors ? 1 : 0,
                    };

                    mPipeline->bind(cmd);
                    mPipeline->pushConstants(cmd, &pushConstants);

                    auto taskGroupSizeX = mShared->hairModels->getHairGeometry(static_cast<size_t>(mShared->config.hairIndex)).taskGroupSizeX;
                    if (mShared->config.useCustomWgSize)
                    {
                        taskGroupSizeX = mShared->config.customTaskWgSize;
                    }
                    cmd->getHandle().drawMeshTasksEXT(taskGroupSizeX, 1, 1);
                });

                pCommandList->endLabel();
            }

            pCommandList->endLabel();
        }

        [[nodiscard]] const SPtr<RHI::Image>& getResult(const uint32_t frameIndex) const
        {
            return mRenderTarget[frameIndex];
        }

    private:
        void createResources()
        {
            for (size_t i = 0; i < mRenderTarget.size(); i++)
            {
                mRenderTarget[i] = makeRenderTarget(mRHI.get(), fmt::format("HybridHair_MeshStage_Target_{}", i));
                mDepthBuffer[i]  = makeRenderTarget(mRHI.get(), fmt::format("HybridHair_MeshStage_Depth_{}", i), vk::Format::eD32Sfloat);
            }
        }

        void createPipeline()
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
                    .label            = fmt::format("HybridHair_MeshStage_RenderPass_", i),
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
                .addShader({ Configuration::getShaderFilePath("HybridHair.task.spv"), vk::ShaderStageFlagBits::eTaskEXT })
                .addShader({ Configuration::getShaderFilePath("HybridHair.mesh.spv"), vk::ShaderStageFlagBits::eMeshEXT })
                .addShader({ Configuration::getShaderFilePath("HybridHair.frag.spv"), vk::ShaderStageFlagBits::eFragment })
                .addColorAttachmentFormat(mRenderTarget[0]->getProperties().format)
                .setDepthAttachmentFormat(mDepthBuffer[0]->getProperties().format)
                .setDebugName("HybridHair_MeshStage_Pipeline");

            mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
        }

        SPtr<RHI::VulkanRHI>                    mRHI;
        HairShared*                             mShared;

        vk::Rect2D                              mScissor;
        vk::Viewport                            mViewport;

        SPtr<RHI::Pipeline>                     mPipeline;
        PerFrameArray<SPtr<RHI::RenderPass>>    mRenderPass;
        PerFrameArray<SPtr<RHI::Image>>         mRenderTarget;
        PerFrameArray<SPtr<RHI::Image>>         mDepthBuffer;
    };
}
