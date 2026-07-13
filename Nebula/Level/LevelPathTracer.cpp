#include "LevelPathTracer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "Level.hpp"
#include "Render/Templates.hpp"

namespace nbl
{
    LevelPathTracer::LevelPathTracer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel)
    : mRHI(rhi)
    , mLevel(pLevel)
    {
        mDescriptor = mRHI->createDescriptor({
            .bindings  = {
                { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 5, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 6, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
                { 7, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR },
            },
            .setCount  = RHI::gFramesInFlight,
            .debugName = "PT_Descriptor",
        });

        for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mCurrentOutput[i]    = makeRenderTarget(mRHI.get(), fmt::format("PT_Output_{}", i), vk::Format::eR32G32B32A32Sfloat);

            mNormals[i]         = makeRenderTarget(mRHI.get(), fmt::format("PT_Normals_{}",    i));
            mDiffuseAlbedo[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_DiffAlbedo_{}", i));
            mSpecularAlbedo[i]  = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecAlbedo_{}", i));
            mRoughness[i]       = makeRenderTarget(mRHI.get(), fmt::format("PT_Roughness_{}",  i), vk::Format::eR16Sfloat);
            mDepth[i]           = makeRenderTarget(mRHI.get(), fmt::format("PT_LinDepth_{}",   i), vk::Format::eR32Sfloat);
            mMotionVectors[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_MVec_{}",       i), vk::Format::eR32G32Sfloat);
            mSpecularHitDist[i] = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecHitDist_{}",i), vk::Format::eR32Sfloat);

            mRROutput[i]        = makeRenderTarget(mRHI.get(), fmt::format("PT_DLSS_RR_{}",    i));

            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeStorageImage(0, vk::ImageLayout::eGeneral, mCurrentOutput[i])
                .writeStorageImage(1, vk::ImageLayout::eGeneral, mNormals[i])
                .writeStorageImage(2, vk::ImageLayout::eGeneral, mDiffuseAlbedo[i])
                .writeStorageImage(3, vk::ImageLayout::eGeneral, mSpecularAlbedo[i])
                .writeStorageImage(4, vk::ImageLayout::eGeneral, mRoughness[i])
                .writeStorageImage(5, vk::ImageLayout::eGeneral, mDepth[i])
                .writeStorageImage(6, vk::ImageLayout::eGeneral, mMotionVectors[i])
                .writeStorageImage(7, vk::ImageLayout::eGeneral, mSpecularHitDist[i]);
            mDescriptor->write(i, descriptorWrite);
        }

        using enum vk::ShaderStageFlagBits;
        const auto ps     = RHI::RayTracingPS().setMaxDepth(1);
        const auto common = RHI::PipelineCommon()
            .setLabel("PathTracer")
            .addShader("pt.rgen.spv")
            .addShader("pt.miss.spv")
            .addShader("pt.chit.spv")
            .addShader("diffuse.call.spv")
            .addShader("mirror.call.spv")
            .addShader("dielectric.call.spv")
            .addDescriptorLayout(0, mDescriptor.get())
            .addDescriptorLayout(1, mLevel->mTlasSystem->getDescriptor().get())
            .addDescriptorLayout(2, mLevel->mTextureManager->getDescriptor().get())
            .setPushConstant<LevelPathTracerPushConstants>(eRaygenKHR | eClosestHitKHR | eMissKHR | eCallableKHR);
        mPipeline = mRHI->createRayTracingPipeline2(ps, common);

        mTonemapPass = makeUnique<TonemapPass>(Tonemap_Params {
            .outputFormat = vk::Format::eR32G32B32A32Sfloat,
            .rhi          = mRHI,
        });

    }

    void LevelPathTracer::render(const RHI::FrameData& frameData, RHI::CommandList* pCommandList)
    {
        pCommandList->beginLabel("PathTracerView::onRender()");

        mTotalFrames++;
        const auto idx = static_cast<uint32_t>(mTotalFrames);
        mJitterX = halton(idx + 1, 2) - 0.5f;
        mJitterY = halton(idx + 1, 3) - 0.5f;

        LevelPathTracerPushConstants pushConstants = {
            .camera         = mLevel->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .prevCamera     = mLevel->mCameraSystem->getPreviousBuffer(frameData.currentFrame)->getAddress(),
            .instances      = mLevel->mInstanceSystem->getBuffer()->getAddress(),
            .emitters       = mLevel->mEmittersBuffer->getAddress(),
            .emitterPdfs    = mLevel->mDiscretePDFsBuffer->getAddress(),
            .vertices       = mLevel->mGeometrySystem->getBuffers().getVertexBuffer()->getAddress(),
            .indices        = mLevel->mGeometrySystem->getBuffers().getIndexBuffer()->getAddress(),
            .materials      = mLevel->mMaterialSystem->getBuffer()->getAddress(),
            .geometryInfos  = mLevel->mGeometrySystem->getBuffers().getInfoBuffer()->getAddress(),
            .accumulated    = 0,
            .totalFrames    = mTotalFrames,
            .maxBounces     = static_cast<uint32_t>(4),
            .spp            = static_cast<uint32_t>(1),
            .bDynamicRR     = 0,
            .rrCont         = 0.7,
            .emitterCount   = static_cast<uint32_t>(mLevel->mEmitters.size()),
            .jitterX        = mJitterX,
            .jitterY        = mJitterY,
        };

        RHI::Barrier()
            .addBarrier(mCurrentOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mLevel->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(mLevel->mTlasSystem->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
            .addBarrier(mNormals[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mDiffuseAlbedo[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mSpecularAlbedo[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mRoughness[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mDepth[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mMotionVectors[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mSpecularHitDist[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .insert(pCommandList);

        pCommandList->bindPipeline(mPipeline.get());
        pCommandList->bindDescriptorSets({
            mDescriptor->getSet(frameData.currentFrame),
            mLevel->mTlasSystem->getDescriptor()->getSet(frameData.currentFrame),
            mLevel->mTextureManager->getDescriptor()->getSet(frameData.currentFrame),
        });
        pCommandList->pushConstants(&pushConstants);

        const auto [w, h] = mCurrentOutput[frameData.currentFrame]->getProperties().extent;
        pCommandList->traceRays(w, h, 1);

        RHI::Barrier()
            .addBarrier(mCurrentOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mNormals[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mDiffuseAlbedo[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mSpecularAlbedo[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mRoughness[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mDepth[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mMotionVectors[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mSpecularHitDist[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .addBarrier(mRROutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::General))
            .insert(pCommandList);

        auto resColor       = wrapImage(mCurrentOutput[frameData.currentFrame], false);
        auto resNormal      = wrapImage(mNormals[frameData.currentFrame], false);
        auto resDiffAlb     = wrapImage(mDiffuseAlbedo[frameData.currentFrame], false);
        auto resSpecAlb     = wrapImage(mSpecularAlbedo[frameData.currentFrame], false);
        auto resRough       = wrapImage(mRoughness[frameData.currentFrame], false);
        auto resDepth       = wrapImage(mDepth[frameData.currentFrame], false);
        auto resMotion      = wrapImage(mMotionVectors[frameData.currentFrame], false);
        auto resSpecHitDist = wrapImage(mSpecularHitDist[frameData.currentFrame], false);
        auto resOutput      = wrapImage(mRROutput[frameData.currentFrame], true);

        const auto& cam = mLevel->mCameraSystem->getActiveCamera()->getGpuCameraData();
        const glm::mat4 view = cam.view;
        const glm::mat4 proj = cam.proj;

        NVSDK_NGX_VK_DLSSD_Eval_Params eval {};

        eval.pInColor               = &resColor;
        eval.pInOutput              = &resOutput;
        eval.pInDepth               = &resDepth;
        eval.pInMotionVectors       = &resMotion;
        eval.pInNormals             = &resNormal;
        eval.pInRoughness           = &resRough;
        eval.pInDiffuseAlbedo       = &resDiffAlb;
        eval.pInSpecularAlbedo      = &resSpecAlb;
        eval.pInSpecularHitDistance = &resSpecHitDist;

        eval.InJitterOffsetX   = -mJitterX;
        eval.InJitterOffsetY   = -mJitterY;
        eval.InRenderSubrectDimensions = { mRROutput[0]->getProperties().extent.width, mRROutput[0]->getProperties().extent.height };
        eval.InMVScaleX        = 1.0f;
        eval.InMVScaleY        = 1.0f;

        static bool firstFrame = true;
        eval.InReset = firstFrame ? 1 : 0;
        firstFrame = false;

        eval.pInWorldToViewMatrix = const_cast<float*>(glm::value_ptr(view));
        eval.pInViewToClipMatrix  = const_cast<float*>(glm::value_ptr(proj));

        const auto r = NGX_VULKAN_EVALUATE_DLSSD_EXT(
            static_cast<VkCommandBuffer>(pCommandList->getHandle()),
            mRHI->getDLSSdFeature(),
            mRHI->getNGXParams(),
            &eval);

        if (NVSDK_NGX_FAILED(r))
        {
            spdlog::error("DLSS-RR eval failed: 0x{:x}", static_cast<uint32_t>(r));
        }

        mTonemapPass->execute(mRROutput[frameData.currentFrame], pCommandList, frameData);
        // mFXAAPass->execute(pCommandList, frameData);

        pCommandList->blitToSwapchain(mTonemapPass->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);

        pCommandList->endLabel();
    }

    NVSDK_NGX_Resource_VK LevelPathTracer::wrapImage(const SPtr<RHI::Image>& img, const bool readWrite)
    {
        const auto&             p = img->getProperties();
        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        return NVSDK_NGX_Create_ImageView_Resource_VK(
            static_cast<VkImageView>(img->getImageView()),
            static_cast<VkImage>(img->getImage()),
            range,
            static_cast<VkFormat>(p.format),
            p.extent.width,
            p.extent.height,
            readWrite);
    }

    float LevelPathTracer::halton(uint32_t index, const uint32_t base)
    {
        float f = 1.0f, r = 0.0f;
        while (index > 0)
        {
            f     /= static_cast<float>(base);
            r     += f * static_cast<float>(index % base);
            index /= base;
        }
        return r;
    }
}
