#include "LevelRenderer.hpp"

#include "LightingPass.hpp"
#include "Templates.hpp"
#include "Level/Level.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace nbl
{
    LevelRenderer::LevelRenderer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel)
    : mRHI(rhi)
    , mTextureManager(pTextureManager)
    , mLevel(pLevel)
    {
        mGBufferPass = GBufferPass::create({
            .pTextureManager = mTextureManager,
            .pLevel          = mLevel,
            .rhi             = mRHI,
        });

        mLightingPass = LightingPass::create({
            .pGBufferPass    = mGBufferPass,
            .pTextureManager = mTextureManager,
            .pLevel          = mLevel,
            .rhi             = mRHI,
        });

        mTonemapPass = TonemapPass::create({
            .color = mLightingPass->getResult(),
            .rhi   = mRHI,
        });

        mBoundingBoxDebugPass = BoundingBoxDebugPass::create({
            .pLevel             = mLevel,
            .renderTarget       = mTonemapPass->getResult(),
            .gBufferDepthBuffer = mGBufferPass->getDepthBuffer(),
            .rhi                = mRHI,
        });
    }

    void LevelRenderer::render(const RHI::FrameData& frameData, const RHI::CommandList* commandList) const
    {
        mGBufferPass->execute(commandList, frameData);
        mLightingPass->execute(commandList, frameData);
        mTonemapPass->execute(commandList, frameData);
        mBoundingBoxDebugPass->execute(commandList, frameData);

        blitToSwapchain(mTonemapPass->getResult().get(), commandList, frameData);
    }

    void LevelRenderer::blitToSwapchain(RHI::Image* pImage, const RHI::CommandList* commandList, const RHI::FrameData& frameData) const
    {
        commandList->beginLabel("Present_Blit");
        // Barriers
        const auto barrier = RHI::Barrier()
            .addBarrier(pImage->getBarrier(RHI::ImageUsage::TransferSrc))
            .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
        barrier.insert(commandList);

        // Blit
        #pragma region
        const auto srcExtent = pImage->getProperties().extent;
        const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
        const auto region  = vk::ImageBlit2()
            .setSrcOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
            })
            .setSrcSubresource(pImage->getProperties().getSubresourceLayers())
            .setDstOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
            })
            .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

        const auto blit = vk::BlitImageInfo2()
            .setSrcImage(pImage->getImage())
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegions(region);
        #pragma endregion
        commandList->getHandle().blitImage2(blit);

        commandList->endLabel();
    }
}
