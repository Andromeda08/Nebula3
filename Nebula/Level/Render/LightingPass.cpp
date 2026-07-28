#include "LightingPass.hpp"

#include <imgui.h>
#include "GBufferPass.hpp"
#include "Templates.hpp"

namespace nbl
{
    LightingPass::LightingPass(const LightingPass_Params& params)
    : mRHI(params.rhi)
    , mInput(params)
    {
        init();

        mInput.pLevel->mUserInterface->addComponent<LightingPassUI>(this);
    }

    void LightingPass::execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("LightingPass::execute()");

        const auto* level   = mInput.pLevel;
        const auto& pOutput = mLightingResult[frameData.currentFrame];
        auto barriers = RHI::Barrier()
            .addBarrier(pOutput->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mInput.pGBufferPass->mWorldPosition->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mWorldNormal->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mAlbedoBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mParamsBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mViewZ->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(level->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect));

        if (mRHI->getFeatures().rayTracing)
        {
            barriers.addBarrier(level->mTlasSystem->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse));
        }
        if (mInput.pSSAOPass)
        {
            barriers.addBarrier(mInput.pSSAOPass->getResult(frameData.currentFrame)->getBarrier(RHI::ImageUsage::ShaderReadOnly));
        }

        barriers.insert(pCommandList);

        const auto& geometryBuffers = level->mGeometrySystem->getBuffers();

        const PushConstants pushConstants = {
            .instances              = level->mInstanceSystem->getBuffer()->getAddress(),
            .instanceIndirectionMap = level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getAddress(),
            .camera                 = level->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .materials              = level->mMaterialSystem->getBuffer()->getAddress(),
            .lights                 = level->mLightSystem->getBuffer()->getAddress(),
            .vertexBuffer           = geometryBuffers.getVertexBuffer()->getAddress(),
            .indexBuffer            = geometryBuffers.getIndexBuffer()->getAddress(),
            .geometryBuffer         = geometryBuffers.getInfoBuffer()->getAddress(),
            .shadowsEnabled         = mEnableShadows ? 1 : 0,
            .sampleCount            = mSampleCount,
            .enableGI               = mEnableGI ? 1 : 0,
            .ambientFactor          = mAmbientFactor,
            .shadowFactor           = mShadowFactor,
            .emissiveFactor         = mEmissiveFactor,
        };
        const PushConstantsNew pcs = {
            .camera = level->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .lights = level->mLightSystem->getBuffer()->getAddress(),
        };

        std::vector descriptors = { mDescriptor->getSet(frameData.currentFrame), mInput.pTextureManager->getDescriptor()->getSet(0) };
        if (mRHI->getFeatures().rayTracing)
        {
            descriptors.push_back(mInput.pLevel->mTlasSystem->getDescriptor()->getSet(frameData.currentFrame));
        }

        RHI::Rendering()
            .setLabel("Lighting_RenderPass")
            .setRenderArea(pOutput->getProperties().extent)
            .addAttachment(pOutput)
            .setViewportScissor(pCommandList)
            .execute(pCommandList, [&](RHI::CommandList* cmd) -> void {
                cmd->bindPipeline(mPipeline.get());
                cmd->bindDescriptorSet(mDescriptor->getSet(frameData.currentFrame), 0);
                cmd->pushConstants(&pcs);
                cmd->getHandle().draw(3, 1, 0, 0);
            });

        pCommandList->endLabel();
    }

    void LightingPass::init() noexcept
    {
        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
                { 5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = RHI::gFramesInFlight,
            .debugName = "Lighting_Descriptor",
        });

        for (size_t i = 0; i < mLightingResult.size(); i++)
        {
            auto descriptorWrite = RHI::DescriptorWrite()
                .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mWorldPosition)
                .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mWorldNormal)
                .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mAlbedoBuffer)
                .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mParamsBuffer)
                .writeCombinedImageSampler(4, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mViewZ);

            if (mInput.pSSAOPass)
            {
                descriptorWrite.writeCombinedImageSampler(5, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pSSAOPass->getResult(i));
            }

            mDescriptor->write(i, descriptorWrite);

            mLightingResult[i] = makeRenderTarget(mRHI.get(), fmt::format("Lighting_Result_{}", i));
        }

        const auto graphicsPS = RHI::GraphicsPS()
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addDefaultAttachmentState(1)
            .addAttachmentFormat(mLightingResult[0]->getProperties().format);
        auto pipelineInfo = RHI::PipelineCommon()
            .setLabel("Lighting")
            .addShader("FSQuad.vert.spv")
            .addShader("LightingV2.frag.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            // .addDescriptorLayout(1, mInput.pTextureManager->getDescriptor().get())
            .setPushConstant<PushConstantsNew>(vk::ShaderStageFlagBits::eFragment);

        if (mRHI->getFeatures().rayTracing)
        {
            pipelineInfo.addDescriptorLayout(2, mInput.pLevel->mTlasSystem->getDescriptor().get());
        }

        mPipeline = mRHI->createGraphicsPipeline2(graphicsPS, pipelineInfo);
    }

    LightingPassUI::LightingPassUI(LightingPass* pPass)
    : IComponent()
    , mPass(pPass)
    {
    }

    void LightingPassUI::draw()
    {
        if (!mPass)
        {
            return;
        }

        ImGui::Begin("Lighting Params");

        ImGui::Checkbox("Shadows", &mPass->mEnableShadows);
        ImGui::Checkbox("GI", &mPass->mEnableGI);

        ImGui::PushItemWidth(64.0f);

        ImGui::DragInt("Samples", &mPass->mSampleCount, 1, 0, 1024);
        ImGui::DragFloat("Emissive Factor", &mPass->mEmissiveFactor, 0.1f, 0.0f, 32.0f);
        // ImGui::DragFloat("Ambient Factor", &mPass->mAmbientFactor, 0.005f, 0.0f, 1.0f);
        // ImGui::DragFloat("Shadow Factor", &mPass->mShadowFactor, 0.005f, 0.0f, 1.0f);

        ImGui::PopItemWidth();

        ImGui::End();
    }
}
