#include "SceneV2.hpp"

#include <glm/gtc/noise.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "GLTF/GLTFLoader.hpp"
#include "Window/SplashWindow.hpp"

SceneV2::SceneV2(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, UserInterface* pUI)
: mRHI(rhi)
, mUserInterface(pUI)
, mTextureManager(pTextureManager)
{
    mGeometry = makeUnique<SceneGeometry>(mRHI);
    mInstancePool = makeUnique<InstancePool>(mRHI, 65536);
    mLightSystem = makeUnique<LightSystem>(mRHI);
    mMaterialPool = makeUnique<MaterialPool>(mRHI, "Material", 4096);

    if (mRHI->getFeatures().rayTracing)
    {
        mTLASManager = TLASManager::create({ mRHI, mInstancePool.get() });
    }

    for (auto&& [i, buffer] : nbl::enumerate(mCameraUniformBuffers))
    {
        buffer = mRHI->createBuffer({
            .size  = sizeof(CameraData),
            .type  = RHI::BufferType::Uniform,
            .label = std::format("Scene_Uniform_Camera_{}", i),
        });
    }

    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        const auto cubeIdx   = mGeometry->addGeometry<Cube>(Cube::Params {});
        mGeometry->commit();

        for (int32_t i = 0; i < 1024; i++)
        {
            auto hMat = mMaterialPool->acquire({
                .solidColor = Random::getColor(),
            });

            const auto base = Random::get(1.0f, 3.5f);
            auto transform = Transform()
                .setTranslate(glm::vec3(Random::get(-128, 128), Random::get(0.0f, 35.0f), Random::get(-128, 128)))
                .setScale(glm::vec3(base));
            (i % 2 == 0)
                ? addObject<Object>(cubeIdx, transform, hMat, fmt::format("Cube#{}", i))
                : addObject<ExampleObject>(cubeIdx, transform, hMat, fmt::format("Cube#{}", i));
        }

        GLTFLoader::loadParts({
            .pTextureManager = mTextureManager,
            .pSceneGeometry  = mGeometry.get(),
            .pLightSystem    = mLightSystem.get(),
            .pMaterialPool   = mMaterialPool.get(),
            .pScene          = this,
        }, { "bistro.glb" });
    }

    using enum vk::ShaderStageFlagBits;
    vk::ShaderStageFlags shaderStageFlags = eVertex | eFragment | eCompute;
    if (mRHI->getFeatures().meshShaders)
    {
        shaderStageFlags |= eMeshEXT | eTaskEXT;
    }
    if (mRHI->getFeatures().rayTracing)
    {
        shaderStageFlags |= eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR;
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { SceneBindings_Camera, vk::DescriptorType::eUniformBuffer, 1, shaderStageFlags },
        { SceneBindings_Lights, vk::DescriptorType::eStorageBuffer, 1, shaderStageFlags },
        { SceneBindings_Materials, vk::DescriptorType::eStorageBuffer, 1, shaderStageFlags },
    };
    if (mRHI->getFeatures().rayTracing)
    {
        bindings.push_back({ 3, vk::DescriptorType::eAccelerationStructureKHR, 1, shaderStageFlags });
    }

    mSceneDescriptor = mRHI->createDescriptor({ bindings, RHI::gFramesInFlight, "Scene_Descriptor" });
    for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
    {
        auto descriptorWrite = RHI::DescriptorWrite()
            .writeUniformBuffer(SceneBindings_Camera, mCameraUniformBuffers[i])
            .writeStorageBuffer(SceneBindings_Lights, mLightSystem->getDataBuffer())
            .writeStorageBuffer(SceneBindings_Materials, mMaterialPool->getBuffer());

        if (mRHI->getFeatures().rayTracing)
        {
            descriptorWrite.writeAccelerationStructure(SceneBindings_TopLevelAS, mTLASManager->getTLAS());
        }

        mSceneDescriptor->write(i, descriptorWrite);
    }

    #pragma region "RenderPass"
    const auto extent = mRHI->getSwapchain()->getProperties().extent;
    mGBufferPass = Indirect_GBufferPass::create({
        .resolution ={ extent.width, extent.height },
        .pScene = this,
        .rhi = mRHI,
    });

    mSSAO = SSAOPass::create({
        .useBlur    = true,
        .resolution = { extent.width, extent.height },
        .input      = { mGBufferPass->getPosition(), mGBufferPass->getNormal(), mSceneDescriptor },
        .rhi        = mRHI,
    });

    mProcSky = ProceduralSkyPass::create({
        .initialParams = SkyParams::fromTimeOfDay(16.0f),
        .rhi = mRHI,
    });
    mUserInterface->addComponent<ProceduralSkyPassComponent>(mProcSky.get());

    mLightingPass = LightingPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mGBufferPass->getPosition(), mGBufferPass->getNormal(), mGBufferPass->getAlbedo(), mSceneDescriptor, mSSAO->getResult(), mGBufferPass->getLightingParams(), mTLASManager.get(), mProcSky->getCubeMap(), mProcSky->getSkyDataBuffer() },
        .rhi        = mRHI,
    });

    mBloomPass = BloomPass::create({
        .resolution = { extent.width, extent.height },
        .input = Bloom_Input {
            .emissive = mGBufferPass->getEmissive(),
            .lighting = mLightingPass->getResult(),
        },
        .rhi = rhi
    });

    mTonemapPass = TonemapPass::create({
        .resolution = { extent.width, extent.height },
        .rhi        = mRHI,
        .input      = { mBloomPass->getResult() },
    });

    mFXAA = FXAAPass::create({
        .resolution = { extent.width, extent.height },
        .input      = { mTonemapPass->getResult() },
        .rhi        = mRHI,
    });

    mAABBPass = AABBOverlayPass::create({
        .input      = { mFXAA->getResult(), this, mGBufferPass->getDepth() },
        .resolution = { extent.width, extent.height },
        .rhi        = mRHI,
    });

    if (mRHI->getFeatures().rayTracing)
    {
        mRTAO = RTAOPass::create({
            .resolution = { extent.width, extent.height },
            .input      = { mGBufferPass->getPosition(), mGBufferPass->getNormal(), mSceneDescriptor },
            .rhi        = mRHI,
        });

        mRTPass = FullRTPass::create({
            .sceneDescriptor = mSceneDescriptor,
            .resolution      = { extent.width, extent.height },
            .rhi             = mRHI,
        });
    }
    #pragma endregion

    initializeObjectSelectionFeature();
}

void SceneV2::onEvent(const SDL_Event& event) noexcept
{
    if (mCamera)
    {
        mCamera->onEvent(event);
    }

    if (mObjSelectPipeline != nullptr && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (const auto& mouseEvent = event.button; mouseEvent.button == SDL_BUTTON_RIGHT)
        {
            mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
                const auto [w, h] = mRHI->getSwapchain()->getProperties().extent;
                glm::vec2 mousePos;
                SDL_GetMouseState(&mousePos.x, &mousePos.y);

                const auto pushConstants = ObjSelectPushConstant {
                    .instanceAddress = mInstancePool->getBuffer()->getAddress(),
                    .selectAddress   = mObjSelectBuffer->getAddress(),
                    .mousePos        = std::move(mousePos),
                    .screenSize      = glm::vec2(w, h),
                };

                mObjSelectPipeline->bind(pCommandList);
                mObjSelectPipeline->bindDescriptorSet(pCommandList, mSceneDescriptor->getSet(0));
                mObjSelectPipeline->pushConstants(pCommandList, &pushConstants);
                mObjSelectPipeline->dispatch(pCommandList, 1);

                RHI::Barrier()
                    .addBarrier(mObjSelectBuffer->getBarrier(RHI::BufferUsage::Compute_Write, RHI::BufferUsage::Host_Read))
                    .insert(pCommandList);
            });

            const auto* pSelectedObj = static_cast<int32_t*>(mObjSelectBuffer->map());
            mSelectedObject = pSelectedObj ? *pSelectedObj : -1;
            spdlog::info("Selected object: {}", mSelectedObject);
        }
    }
}

void SceneV2::onUpdate(const float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept
{
    static bool isFirstUpdate = true;

    // Update materials
    mMaterialPool->flush(pCommandList);

    // Update Instance Data
    for (const auto& obj : mObjects)
    {
        obj->onUpdate(dt);
        if (obj->transform.isDirty() || isFirstUpdate)
        {
            auto data = obj->getInstanceData(mMaterialPool.get());

            data.blasAddress = mGeometry->getBlasAddress(data.geometryIndex);
            obj->boundingBox = mGeometry->getGeometry(data.geometryIndex)->getBoundingBox().getTransformed(data.model);
            data.min = glm::vec4(obj->boundingBox.getMin(), 1.0f);
            data.max = glm::vec4(obj->boundingBox.getMax(), 1.0f);

            mInstancePool->update(obj->instanceIndex, data);
        }
    }
    mInstancePool->flush(pCommandList, frameData.currentFrame);

    // Update Top-Level AS
    if (mRHI->getFeatures().rayTracing)
    {
        mTLASManager->onUpdate(pCommandList);

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeAccelerationStructure(SceneBindings_TopLevelAS, mTLASManager->getTLAS());
        mSceneDescriptor->write(frameData.currentFrame, descriptorWrite);
    }

    // Update Camera Data
    if (mCamera)
    {
        mCamera->onUpdate();

        const auto cameraData = mCamera->getCameraData();
        mCameraUniformBuffers[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
    }

    // Update draw commands
    if (!mObjects.empty())
    {
        buildDrawCommands(pCommandList, frameData);
    }

    mObjectCountChanged = false;
    isFirstUpdate       = false;
}

void SceneV2::onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept
{
    mProcSky->execute(commandList, frameData);

    mGBufferPass->execute(commandList, frameData);
    mSSAO->execute(commandList, frameData);
    // mRTAO->execute(commandList, frameData);
    mLightingPass->execute(commandList, frameData);
    mBloomPass->execute(commandList, frameData);
    mTonemapPass->execute(commandList, frameData);
    mFXAA->execute(commandList, frameData);

    if (mVisualizeAABBs)
    {
        mAABBPass->execute(commandList, frameData);
    }

    // mRTPass->execute(commandList, frameData);

    const auto presentMe = mAABBPass->getResult();

    commandList->beginLabel("Present_Blit");
    // Barriers
    const auto barrier = RHI::Barrier()
        .addBarrier(presentMe->getBarrier(RHI::ImageUsage::TransferSrc))
        .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
    barrier.insert(commandList);

    // Blit
    #pragma region
    const auto srcExtent = presentMe->getProperties().extent;
    const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
    const auto region  = vk::ImageBlit2()
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
        })
        .setSrcSubresource(presentMe->getProperties().getSubresourceLayers())
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
        })
        .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

    const auto blit = vk::BlitImageInfo2()
        .setSrcImage(presentMe->getImage())
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFilter(vk::Filter::eLinear)
        .setRegions(region);
    #pragma endregion
    commandList->getHandle().blitImage2(blit);

    commandList->endLabel();
}

void SceneV2::initializeObjectSelectionFeature()
{
    if (mRHI->getFeatures().rayTracing)
    {
        spdlog::warn("Object selection feature is not available.");
        return;
    }

    mObjSelectBuffer = mRHI->createBuffer({
        .size  = sizeof(int32_t),
        .type  = RHI::BufferType::Readback,
        .label = "ObjSelectBuffer",
    });

    auto pipelineInfo = RHI::ComputePipelineCreateInfo()
        .addDescriptorSetLayout(mSceneDescriptor->getLayout())
        .setComputeShader(Configuration::getShaderFilePath("RQSelect.comp.spv"))
        .setDebugName("ObjSelectPipeline")
        .setPushConstantRange<ObjSelectPushConstant>(vk::ShaderStageFlagBits::eCompute);
    mObjSelectPipeline = mRHI->createComputePipeline(pipelineInfo);
}

void SceneV2::buildDrawCommands(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) noexcept
{
    std::unordered_map<GeometryIndex, std::vector<uint32_t>> groups;
    for (const auto& obj : mObjects)
    {
        groups[obj->geometryIndex].push_back(obj->instanceIndex);
    }

    // Frustum Cull
    #pragma region
    const auto cameraData = mCamera->getCameraData();
    const auto vp = cameraData.proj * cameraData.view;

    const glm::vec4 row0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
    const glm::vec4 row1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
    const glm::vec4 row2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    const glm::vec4 row3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

    std::array frustumPlanes = {
        row3 + row0,  // left
        row3 - row0,  // right
        row3 + row1,  // bottom
        row3 - row1,  // top
        row2,         // near
        row3 - row2,  // far
    };

    for (auto& p : frustumPlanes)
    {
        p /= glm::length(glm::vec3(p));
    }
    #pragma endregion

    uint32_t totalInstanceCount        = 0;
    uint32_t totalVisibleInstanceCount = 0;

    std::vector<uint32_t>                       instanceMap;
    std::vector<vk::DrawIndexedIndirectCommand> draws;

    auto cullTime = DeltaTime().initialize();
    for (auto& [geometryIndex, instanceIndices] : groups)
    {
        // Redundant query, fix later, obj already has this data
        auto geometryView = mGeometry->getGeometryView(geometryIndex);

        const auto firstInstance = static_cast<uint32_t>(instanceMap.size());

        // Culling
        uint32_t visibleInstanceCount = 0;
        for (auto instanceIndex : instanceIndices)
        {
            auto shouldKeep = !mEnableCulling;
            if (mEnableCulling)
            {
                const auto& instanceData = mInstancePool->getData().at(instanceIndex);
                shouldKeep = nbl::BoundingBox(instanceData.min, instanceData.max).isVisible(frustumPlanes);
            }
            if (shouldKeep)
            {
                visibleInstanceCount += 1;
                instanceMap.push_back(instanceIndex);
            }
        }

        totalInstanceCount        += instanceIndices.size();
        totalVisibleInstanceCount += visibleInstanceCount;

        const auto cmd = vk::DrawIndexedIndirectCommand()
            .setIndexCount(geometryView.metadata->indexCount)
            .setInstanceCount(visibleInstanceCount)
            .setFirstIndex(geometryView.metadata->firstIndex)
            .setVertexOffset(static_cast<int32_t>(geometryView.metadata->firstVertex))
            .setFirstInstance(firstInstance);
        draws.push_back(cmd);
    }

    mLastCull  = CullStats::make(totalInstanceCount, totalVisibleInstanceCount, cullTime.getDeltaTime());
    mDrawCount = mDrawCount = static_cast<uint32_t>(draws.size());;

    const auto drawSize = mDrawCount * sizeof(vk::DrawIndexedIndirectCommand);
    mDrawCmdBuffer[frameData.currentFrame] = mRHI->createBuffer({
        .size  = drawSize,
        .type  = RHI::BufferType::Indirect,
        .label = fmt::format("Scene_Draw_Commands_{}", frameData.currentFrame),
    });

    const auto mapSize = instanceMap.size() * sizeof(uint32_t);
    mInstanceMapBuffer[frameData.currentFrame] = mRHI->createBuffer({
        .size  = mapSize,
        .type  = RHI::BufferType::Storage,
        .label = fmt::format("Scene_Instance_Map_{}", frameData.currentFrame),
    });

    mDrawStaging[frameData.currentFrame] = mRHI->createBuffer({
        .size  = drawSize + mapSize,
        .type  = RHI::BufferType::Staging,
        .label = "Scene_Staging",
    });
    mDrawStaging[frameData.currentFrame]->setData(draws.data(), drawSize, 0);
    mDrawStaging[frameData.currentFrame]->setData(instanceMap.data(), mapSize, drawSize);

    const auto drawCopy     = vk::BufferCopy2 { 0, 0, drawSize };
    const auto drawCopyInfo = vk::CopyBufferInfo2()
        .setSrcBuffer(mDrawStaging[frameData.currentFrame]->getHandle())
        .setDstBuffer(mDrawCmdBuffer[frameData.currentFrame]->getHandle())
        .setRegions(drawCopy);
    pCommandList->getHandle().copyBuffer2(drawCopyInfo);

    const auto mapRegion   = vk::BufferCopy2 { drawSize, 0, mapSize };
    const auto mapCopyInfo = vk::CopyBufferInfo2()
        .setSrcBuffer(mDrawStaging[frameData.currentFrame]->getHandle())
        .setDstBuffer(mInstanceMapBuffer[frameData.currentFrame]->getHandle())
        .setRegions(mapRegion);
    pCommandList->getHandle().copyBuffer2(mapCopyInfo);
}
