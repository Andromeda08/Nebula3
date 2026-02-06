#include "vxlRenderPath.hpp"

#include "Scene/Voxel/VoxelScene.hpp"

vxlRenderPath::vxlRenderPath(const SPtr<RHI::VulkanRHI>& rhi, VoxelScene* pScene)
: mScene(pScene)
, mRHI(rhi)
{
    mRenderExtent     = mRHI->getSwapchain()->getProperties().extent;
    mRenderResolution = { mRenderExtent.width, mRenderExtent.height };

    mGBuffer = Voxel_GBufferPass::create({
        .resolution = mRenderResolution,
        .pScene     = mScene,
        .rhi        = mRHI,
    });

    mSSAO = SSAOPass::create({
        .useBlur    = true,
        //.resolution = { mRenderResolution.width / 2, mRenderResolution.height / 2 },
        .resolution = mRenderResolution,
        .input      = { mGBuffer->getPosition(), mGBuffer->getNormal(), mScene->mSceneDescriptor },
        .rhi        = mRHI,
    });

    mLightingPass = LightingPass::create({
        .resolution = mRenderResolution,
        .input      = { mGBuffer->getPosition(), mGBuffer->getNormal(), mGBuffer->getAlbedo(), mScene->mSceneDescriptor, mSSAO->getResult() },
        .rhi        = mRHI,
    });
}

void vxlRenderPath::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
    commandList->beginLabel("Voxel_RenderPath");

    mGBuffer->execute(commandList, frameData);
    mSSAO->execute(commandList, frameData);
    mLightingPass->execute(commandList, frameData);

    // Blit final image to swapchain
    execute_BlitToSwapchain(mLightingPass->getResult().get(), commandList, frameData);

    commandList->endLabel();
}

void vxlRenderPath::execute_BlitToSwapchain(RHI::Image* pFinalImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept
{
    commandList->beginLabel("vxlFinalImageBlit");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(pFinalImage->getBarrier(RHI::ImageUsage::TransferSrc))
        .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
    barrier.insert(commandList);

    // Blit
    const auto srcExtent = pFinalImage->getProperties().extent;
    const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
    const auto region    = vk::ImageBlit2()
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
        })
        .setSrcSubresource(pFinalImage->getProperties().getSubresourceLayers())
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
        })
        .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

    const auto blit = vk::BlitImageInfo2()
        .setSrcImage(pFinalImage->getImage())
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFilter(vk::Filter::eLinear)
        .setRegions(region);

    commandList->getHandle().blitImage2(blit);
    commandList->endLabel();
}
