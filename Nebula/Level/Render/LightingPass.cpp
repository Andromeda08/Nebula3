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

    void LightingPass::execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept
    {
        pCommandList->beginLabel("LightingPass");

        const auto* level   = mInput.pLevel;
        auto barriers = RHI::Barrier()
            .addBarrier(mLightingResult->getBarrier(RHI::ImageUsage::ColorAttachment))
            .addBarrier(mInput.pGBufferPass->mWorldPosition->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mWorldNormal->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mAlbedoBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mParamsBuffer->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(mInput.pGBufferPass->mViewZ->getBarrier(RHI::ImageUsage::ShaderReadOnly))
            .addBarrier(level->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mInstanceIndirectionMapBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::StorageRead))
            .addBarrier(level->mDrawCommandsBuffer[frameData.currentFrame]->getBarrier(RHI::BufferUsage::TransferDst, RHI::BufferUsage::DrawIndirect));

        if (mRHI->getRaytracingSupport())
        {
            barriers.addBarrier(level->mTlasSystem->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse));
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

        std::vector descriptors = { mDescriptor->getSet(0), mInput.pTextureManager->getDescriptor()->getSet(0) };
        if (mRHI->getRaytracingSupport())
        {
            descriptors.push_back(mInput.pLevel->mTlasSystem->getDescriptor()->getSet(frameData.currentFrame));
        }

        mRenderPass->execute(pCommandList, [&](const RHI::CommandList* cmd) -> void {
            cmd->setViewportScissor(mViewport, mScissor);

            mPipeline->bind(cmd);
            mPipeline->bindDescriptorSets(cmd, descriptors);
            mPipeline->pushConstants(cmd, &pushConstants);
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
           },
           .setCount = 1,
           .debugName = "Lighting_Descriptor",
       });

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSampler(0, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mWorldPosition)
            .writeCombinedImageSampler(1, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mWorldNormal)
            .writeCombinedImageSampler(2, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mAlbedoBuffer)
            .writeCombinedImageSampler(3, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mParamsBuffer)
            .writeCombinedImageSampler(4, 0, vk::ImageLayout::eShaderReadOnlyOptimal, mInput.pGBufferPass->mViewZ);
        mDescriptor->write(0, descriptorWrite);

        mLightingResult = makeRenderTarget(mRHI.get(), "Lighting_Result");

        mScissor = getRenderAreaForAttachment(mLightingResult.get());
        mViewport = vk::Viewport {
            0.0f, 0.0f,
            static_cast<float>(mScissor.extent.width), static_cast<float>(mScissor.extent.height),
            0.0f, 1.0f
        };

        mRenderPass = mRHI->createRenderPass({
            .renderArea       = mScissor,
            .colorAttachments = { makeAttachment(mLightingResult) },
            .label            = "Lighting_RenderPass",
        });

        auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mDescriptor->getLayout())
            .addDescriptorSetLayout(mInput.pTextureManager->getDescriptor()->getLayout())
            .setPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .setStateInfo(RHI::makeGraphicsStateInfo([&](RHI::GraphicsPipelineStateInfo& stateInfo)
            {
                stateInfo
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .addDefaultAttachmentStates(1);
            }))
            .addShader({ Configuration::getShaderFilePath("FSQuad.vert.spv"), vk::ShaderStageFlagBits::eVertex })
            .addShader({ Configuration::getShaderFilePath("Lighting.frag.spv"), vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mLightingResult->getProperties().format)
            .setDebugName("Lighting_Pipeline");

        if (mRHI->getRaytracingSupport())
        {
            pipelineCreateInfo.addDescriptorSetLayout(mInput.pLevel->mTlasSystem->getDescriptor()->getLayout());
        }

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
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
