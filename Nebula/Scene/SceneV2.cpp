#include "SceneV2.hpp"

#include <glm/gtc/noise.hpp>
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
            .size  = sizeof(CameraData),
            .type  = RHI::BufferType::Uniform,
            .label = std::format("Scene_Uniform_Camera_{}", i),
        });
    }

    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        GLTFLoader::loadParts({
            .pTextureManager = mTextureManager.get(),
            .pSceneGeometry  = mGeometry.get(),
            .pLightSystem    = mLightSystem.get(),
            .pScene          = this,
        }, { "bistro.glb" });

        initCubeScene();
    }

    // initScene();

    using enum vk::ShaderStageFlagBits;
    vk::ShaderStageFlags shaderStageFlags = eVertex | eFragment | eCompute;
    if (mRHI->getMeshShaderSupport())
    {
        shaderStageFlags |= eMeshEXT | eTaskEXT;
    }
    if (mRHI->getRaytracingSupport())
    {
        shaderStageFlags |= eRaygenKHR | eAnyHitKHR | eClosestHitKHR | eMissKHR | eIntersectionKHR | eCallableKHR;
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        { 0, vk::DescriptorType::eUniformBuffer, 1, shaderStageFlags },
        { 1, vk::DescriptorType::eStorageBuffer, 1, shaderStageFlags },
    };
    if (mRHI->getRaytracingSupport())
    {
        bindings.push_back({ 2, vk::DescriptorType::eAccelerationStructureKHR, 1, shaderStageFlags });
    }

    /* TODO: Bindless */ {
        mSceneDescriptor = mRHI->createDescriptor({
            .bindings = bindings,
            .setCount = 2,
            .debugName = "Scene_Descriptor",
        });

        for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
        {
            auto descriptorWrite = RHI::DescriptorWrite()
                .writeUniformBuffer(0, mCameraUniformBuffers[i])
                .writeStorageBuffer(1, mLightSystem->getDataBuffer());

            if (mRHI->getRaytracingSupport())
            {
                descriptorWrite.writeAccelerationStructure(2, mTLASManager->getTLAS());
            }

            mSceneDescriptor->write(i, descriptorWrite);
        }
    }

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
        .input      = { mGBufferPass->getPosition(), mGBufferPass->getNormal(), mGBufferPass->getAlbedo(), mSceneDescriptor, mSSAO->getResult(), mTLASManager.get(), mProcSky->getCubeMap(), mProcSky->getSkyDataBuffer() },
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

    mAABBPass = AABBOverlayPass::create({
        .input      = { mTonemapPass->getResult(), this, mGBufferPass->getDepth() },
        .resolution = { extent.width, extent.height },
        .rhi        = mRHI,
    });

    if (mRHI->getRaytracingSupport())
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
}

void SceneV2::onUpdate(const float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept
{
    static bool isFirstUpdate = true;

    // Update Instance Data
    for (const auto& obj : mObjects)
    {
        obj->onUpdate(dt);
        if (obj->transform.isDirty() || isFirstUpdate)
        {
            mInstancePool->update(obj->instanceIndex, obj->getInstanceData());
        }
    }
    mInstancePool->flush(pCommandList);

    // Update Top-Level AS
    if (mRHI->getRaytracingSupport())
    {
        mTLASManager->onUpdate(pCommandList);

        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeAccelerationStructure(2, mTLASManager->getTLAS());
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
    mFXAA->execute(commandList, frameData);
    mTonemapPass->execute(commandList, frameData);

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
    const auto vp         = cameraData.proj * cameraData.view;
    const auto vpt        = glm::transpose(vp);

    const std::array<glm::vec4, 6> frustumPlanes = {
        // left, right, bottom, top
        (vpt[3] + vpt[0]),
        (vpt[3] - vpt[0]),
        (vpt[3] + vpt[1]),
        (vpt[3] - vpt[1]),
        // near, far
        (vpt[3] + vpt[2]),
        (vpt[3] - vpt[2]),
    };
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
                shouldKeep = BoundingBox::isVisible(instanceData.min, instanceData.max, frustumPlanes);
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
    mDrawCount = mGeometry->getGeometryCount();

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

    mDrawStaging = mRHI->createBuffer({
        .size  = drawSize + mapSize,
        .type  = RHI::BufferType::Staging,
        .label = "Scene_Staging",
    });
    mDrawStaging->setData(draws.data(), drawSize, 0);
    mDrawStaging->setData(instanceMap.data(), mapSize, drawSize);

    const auto drawCopy     = vk::BufferCopy2 { 0, 0, drawSize };
    const auto drawCopyInfo = vk::CopyBufferInfo2()
        .setSrcBuffer(mDrawStaging->getHandle())
        .setDstBuffer(mDrawCmdBuffer[frameData.currentFrame]->getHandle())
        .setRegions(drawCopy);
    pCommandList->getHandle().copyBuffer2(drawCopyInfo);

    const auto mapRegion   = vk::BufferCopy2 { drawSize, 0, mapSize };
    const auto mapCopyInfo = vk::CopyBufferInfo2()
        .setSrcBuffer(mDrawStaging->getHandle())
        .setDstBuffer(mInstanceMapBuffer[frameData.currentFrame]->getHandle())
        .setRegions(mapRegion);
    pCommandList->getHandle().copyBuffer2(mapCopyInfo);
}

void SceneV2::initCubeScene()
{
    mLightSystem->addLight({});

    const auto geoCube = mGeometry->addGeometry<Cube>(Cube::Params {});
    mGeometry->commit();

    const auto cubeInfo = mGeometry->getGeometryView(geoCube);

    constexpr int32_t steps    = 1024;
    constexpr float   stepSize = 1.0f;
    constexpr float   scale    = 0.005f;
    std::vector<glm::vec2> mainPath(steps);
    auto pos = glm::vec2(0.0f, 0.0f);
    for (auto i = 0; i < steps; i++)
    {
        mainPath[i] = pos;

        const float n = glm::perlin(pos * scale);
        const float angle = n * glm::two_pi<float>();

        pos += glm::vec2(glm::cos(angle), glm::sin(angle)) * stepSize;
    }

    std::vector<glm::vec3> cubePos;
    std::vector<glm::vec3> cubeScale;
    /*
    *for (auto i = 0; i < mainPath.size(); i++)
    {
        auto prev = i > 0 ? i - 1 : 0;
        auto next = i < mainPath.size() - 1 ? i + 1 : i;
        glm::vec2 tangent = glm::normalize(mainPath[next] - mainPath[prev]);
        glm::vec2 normal  = { -tangent.y, tangent.x };

        constexpr auto count = 8;
        for (auto j = 0; j <= count; j++)
        {
            float t = (count > 0) ? static_cast<float>(j) / count : 0.5f;
            float offset = -(count * 0.5f) + t * count;
            const auto scale = Random::get(2.0f, 3.0f);

            glm::vec2 pos = mainPath[i] + normal * offset * (scale - 0.05f);

            const float c = static_cast<float>(count) * 0.5f;
            float edgeHeightPenalty = -(c - glm::abs(static_cast<float>(j) - c)) * 0.75f;
            float scaleNorm = scale - 1.0f;
            float heightOffset = Random::get(-0.1f, 0.1f);

            float y = heightOffset - scaleNorm - edgeHeightPenalty;

            cubePos.push_back({ pos.x, y, pos.y });
            cubeScale.push_back(glm::vec3(scale));
        }
    }
     */

    for (auto i = 0; i < mainPath.size(); i++)
    {
        auto prev = i > 0 ? i - 1 : 0;
        auto next = i < mainPath.size() - 1 ? i + 1 : i;
        glm::vec2 tangent = glm::normalize(mainPath[next] - mainPath[prev]);
        glm::vec2 normal  = { -tangent.y, tangent.x };

        constexpr float cubeSize = 2.0f;
        constexpr auto count = 8;

        for (auto j = 0; j <= count; j++)
        {
            float t = static_cast<float>(j) / count;
            float offset = -(count * 0.5f) + t * count;

            glm::vec2 pos = mainPath[i] + normal * offset * cubeSize;
            pos = glm::round(pos / cubeSize) * cubeSize;

            const float c = static_cast<float>(count) * 0.5f;
            float edgeHeightPenalty = -(c - glm::abs(static_cast<float>(j) - c)) * 0.25f;
            float heightOffset = Random::get(-0.1f, 0.1f);
            float y = heightOffset - edgeHeightPenalty;

            cubePos.push_back({ pos.x, y, pos.y });
            cubeScale.push_back(glm::vec3(cubeSize * (Random::unit() > 0.85f ?  Random::get(1.0f, 1.5f) : 1.0f)));
        }
    }

    std::unordered_set<uint64_t> seen;
    std::vector<glm::vec3> uniquePos;
    std::vector<glm::vec3> uniqueScale;

    for (size_t i = 0; i < cubePos.size(); i++)
    {
        int32_t gx = static_cast<int32_t>(glm::round(cubePos[i].x / 2.0f));
        int32_t gz = static_cast<int32_t>(glm::round(cubePos[i].z / 2.0f));
        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(gx)) << 32)
                     | static_cast<uint64_t>(static_cast<uint32_t>(gz));

        if (seen.insert(key).second)
        {
            uniquePos.push_back({ cubePos[i].x, cubePos[i].y - 4.5f, cubePos[i].z });
            uniqueScale.push_back({cubeScale[i].x, 5.0f, cubeScale[i].z});
        }
    }

    cubePos = std::move(uniquePos);
    cubeScale = std::move(uniqueScale);

    for (auto i = 0; i < cubePos.size(); i++)
    {
        auto t = Transform()
            .setTranslate({ cubePos[i].x, cubePos[i].y, cubePos[i].z})
            .setScale(cubeScale[i]);
        addObject<Object>(geoCube, -1, t, glm::vec4(0.0f), glm::vec4(0.0f), -1);
        mObjects.back()->solidColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    }

    auto t = Transform()
        .setTranslate({ 0.0f, -4.0f, 0.0f })
        .setScale({ 512.0f, 0.05f, 512.0f });
    addObject<Object>(geoCube, -1, t, glm::vec4(0.0f), glm::vec4(0.0f), -1);
    mObjects.back()->solidColor = glm::vec4(0.15f, 0.05f, 0.45f, 1.0f);
}
