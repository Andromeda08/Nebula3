#include "SceneV2.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLTF/GLTFLoader.hpp"
#include "Window/SplashWindow.hpp"

SceneV2::SceneV2(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUI)
: mRHI(rhi)
, mUserInterface(pUI)
{
    mGeometry = makeUnique<SceneGeometry>(mRHI);
    mInstancePool = makeUnique<InstancePool>(mRHI, 65536);
    mTextureManager = TextureManager::create({ mRHI });

    if (mRHI->getRaytracingSupport())
    {
        mTLASManager = TLASManager::create({ mRHI, mInstancePool.get() });
    }

    mLightSystem = makeUnique<LightSystem>(mRHI);

    for (auto&& [i, buffer] : nbl::enumerate(mCameraUniformBuffers))
    {
        buffer = mRHI->createBuffer({
            .size = sizeof(CameraData),
            .type = RHI::BufferType::Uniform,
            .label = std::format("Scene_Uniform_Camera_{}", i),
        });
    }

    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        const std::string sceneName = "bistro.glb"; //"NewSponza_Curtains_glTF.gltf";
        SplashWindow::get().setMessage(std::format("Loading Scene ({})...", sceneName));

        GLTFLoader::loadParts({
            .pTextureManager = mTextureManager.get(),
            .pSceneGeometry  = mGeometry.get(),
            .pLightSystem    = mLightSystem.get(),
            .pScene          = this,
        }, { sceneName });
        // }, { "NewSponza_Main_glTF_003.gltf", "NewSponza_Curtains_glTF.gltf" });
    }

    // initScene();

    using enum vk::ShaderStageFlagBits;
    std::vector bindings = {
        vk::DescriptorSetLayoutBinding {
            0, vk::DescriptorType::eUniformBuffer, 1,
            eVertex | eFragment | eCompute | eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR
        },
        vk::DescriptorSetLayoutBinding {
            1, vk::DescriptorType::eStorageBuffer, 1,
            eVertex | eFragment | eCompute | eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR
        },
    };
    if (mRHI->getRaytracingSupport())
    {
        bindings.push_back({
            2, vk::DescriptorType::eAccelerationStructureKHR, 1,
            eVertex | eFragment | eCompute | eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR
        });
    }

    /* TODO: Bindless */ {
        mSceneDescriptor = mRHI->createDescriptor({
            .bindings = bindings,
            .setCount = 2,
            .debugName = "Scene_Descriptor",
        });

        for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
        {
            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeUniformBuffer(0, mCameraUniformBuffers[i])
                .writeStorageBuffer(1, mLightSystem->getDataBuffer())
                .writeAccelerationStructure(2, mTLASManager->getTLAS());
            mSceneDescriptor->write(i, descriptorWrite);
        }
    }

    const auto extent = mRHI->getSwapchain()->getProperties().extent;
    mTestPass = Indirect_GBufferPass::create({
        .resolution ={ extent.width, extent.height },
        .pScene = this,
        .rhi = mRHI,
    });

    mSSAO = SSAOPass::create({
        .useBlur    = true,
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mSceneDescriptor },
        .rhi        = mRHI,
    });

    mRTAO = RTAOPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mSceneDescriptor },
        .rhi        = mRHI,
    });

    mProcSky = ProceduralSkyPass::create({
        .initialParams = SkyParams::fromTimeOfDay(16.0f),
        .rhi = mRHI,
    });
    mUserInterface->addComponent<ProceduralSkyPassComponent>(mProcSky.get());

    mLightingPass = LightingPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mTestPass->getAlbedo(), mSceneDescriptor, mSSAO->getResult(), mProcSky->getCubeMap(), mProcSky->getSkyDataBuffer() },
        .rhi        = mRHI,
    });

    mFXAA = FXAAPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mLightingPass->getResult() },
        .rhi        = mRHI,
    });

    mTonemapPass = TonemapPass::create({
        .resolution = { extent.width, extent.height },
        .rhi        = mRHI,
        .input      = { mFXAA->getResult() },
    });

    mRTPass = FullRTPass::create({
        .sceneDescriptor = mSceneDescriptor,
        .resolution      = { extent.width, extent.height },
        .rhi             = mRHI,
    });
}

void SceneV2::onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept
{
    // mProcSky->execute(commandList, frameData);

    // mTestPass->execute(commandList, frameData);
    // mSSAO->execute(commandList, frameData);
    // mRTAO->execute(commandList, frameData);
    // mLightingPass->execute(commandList, frameData);
    // mFXAA->execute(commandList, frameData);
    // mTonemapPass->execute(commandList, frameData);

    mRTPass->execute(commandList, frameData);

    commandList->beginLabel("Present_Blit");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(mRTPass->getResult()->getBarrier(RHI::ImageUsage::TransferSrc))
        .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
    barrier.insert(commandList);

    // Blit
    #pragma region
    const auto srcExtent = mRTPass->getResult()->getProperties().extent;
    const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
    const auto region  = vk::ImageBlit2()
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
        })
        .setSrcSubresource(mRTPass->getResult()->getProperties().getSubresourceLayers())
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
        })
        .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

    const auto blit = vk::BlitImageInfo2()
        .setSrcImage(mRTPass->getResult()->getImage())
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFilter(vk::Filter::eLinear)
        .setRegions(region);
    #pragma endregion
    commandList->getHandle().blitImage2(blit);

    commandList->endLabel();
}
