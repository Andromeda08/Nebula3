#include "Level.hpp"

#include <unordered_map>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include "TextureManager.hpp"
#include "Core/Random.hpp"
#include "Camera/IsoCamera.hpp"
#include "GLTF/GLTFLoader.hpp"
#include "Level/Camera/FlyingCamera.hpp"
#include "Math/DeltaTime.hpp"
#include "Object/RotatingObject.hpp"
#include "UserInterface/UserInterface.hpp"
#include "Voxel/TerrainGenerator.hpp"
#include "Voxel/Features/FoliageGenerator.hpp"

namespace nbl
{
    Level::Level(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUserInterface, TextureManager* pTextureManager)
    : mRHI(rhi), mUserInterface(pUserInterface), mTextureManager(pTextureManager)
    {
        mCameraSystem = makeUnique<CameraSystem>(rhi);
        mGeometrySystem = makeUnique<GeometrySystem>(rhi);
        mLightSystem = makeUnique<LightSystem>(rhi);

        mMaterialSystem = makeUnique<MaterialSystem>(mRHI, 65536);
        mInstanceSystem = makeUnique<InstanceSystem>(mRHI, 4194304);

        if (mRHI->getFeatures().rayTracing)
        {
            mBlasSystem = makeUnique<BLASSystem>(mRHI, mGeometrySystem.get());
            mTlasSystem = makeUnique<TLASSystem>(mRHI, mInstanceSystem.get());
            mSelectObjectFeature = makeUnique<SelectObjectFeature>(mRHI, mCameraSystem.get(), mInstanceSystem.get(), mTlasSystem.get(), nullptr);
        }

        /* Example Cameras */ {
            const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
            mCameraSystem->addCamera<FlyingCamera>(true, glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));
            mCameraSystem->addCamera<IsometricCamera>(false);
        }

        /* GLTF Scene */ {
            GLTFLoader loader({
                .filePath        = Configuration::getSceneFilePath("bistro.glb"),
                .pLevel          = this,
                .pTextureManager = mTextureManager,
                .pGeometrySystem = mGeometrySystem.get(),
                .pLightSystem    = mLightSystem.get(),
                .pMaterialSystem = mMaterialSystem.get(),
            });

            // loader.load();
        }

        #pragma region "Generate Test Scene"

        const auto hInitLight = mLightSystem->acquire({
            .vector       = glm::vec3(5.0f, 25.0f, 0.0f),
            .color        = glm::vec3(1.0f),
            .intensity    = 15000.0f,
            .isEnabled    = true,
            .castsShadows = true,
            .radius       = 50.0f,
            .type         = LightType::Point,
            .name         = fmt::format("Light {}", mLightSystem->getSize()),
        });

        const int32_t cubeIdx   = mGeometrySystem->addGeometry(Cube::createGeometry());
        const int32_t sphereIdx = mGeometrySystem->addGeometry(Sphere::createGeometry());

        auto terrainGenerator = vxl::TerrainGenerator({ 256, 24, 128, true });
        terrainGenerator.addGenerator<vxl::FoliageGenerator>(vxl::FoliageGenerator::Control{
            .patchCount             = 12,
            .patchRadius            = 12,
            .radiusVariance         = 3,
            .density                = 0.65f,
            .patchDensityVariance   = true,
            .instanceRandomOffset   = true,
            .instanceRandomScale    = true,
        });
        terrainGenerator.generate();

        std::unordered_map<glm::vec3, Handle> voxelMaterial;

        for (const auto& [i, voxel] : nbl::enumerate(terrainGenerator.getResult()))
        {
            if (!voxelMaterial.contains(voxel.color))
            {
                const auto hMat = mMaterialSystem->acquire({
                    .solidColor  = glm::vec4(voxel.color, 1.0f),
                });
                voxelMaterial.insert_or_assign(voxel.color, hMat);
            }

            addObject({
                .geometryIndex = cubeIdx,
                .transform     = Transform().setTranslate(voxel.position).setScale(voxel.scale),
                .hMaterial     = voxelMaterial.at(voxel.color),
                .isEmitter     = false,
            });
        }

        for (int32_t i = 0; i < 512; i++)
        {
            const auto radiance = glm::xyz(Random::getColor());
            const auto hMat = mMaterialSystem->acquire({
                .solidColor  = glm::vec4(radiance, 1.0f),
            });

            auto transform = Transform()
                .setTranslate(terrainGenerator.getResult().at(Random::get<size_t>(0, terrainGenerator.getResult().size() - 1)).position + glm::vec3(0.0f, 2.0f, 0.0f))
                .setScale(glm::vec3(1.0f));

            addObject({
                .geometryIndex = cubeIdx,
                .transform     = transform,
                .hMaterial     = hMat,
                .isEmitter     = true,
                .radiance      = radiance * 50.0f,
            });
        }

        for (int32_t i = 0; i < 512; i++)
        {
            const auto hMat = mMaterialSystem->acquire({
                .solidColor = glm::vec4(1.0f),
                .bsdfIndex  = 1, // Random::get<uint32_t>(1, 2),
            });

            auto transform = Transform()
                .setTranslate(glm::vec3(Random::get(-128, 128), Random::get(-32.0f, 32.0f), Random::get(-128, 128)))
                .setScale(glm::vec3(2.0f));

            addObject({
                .geometryIndex = Random::get<int32_t>() % 2 == 0 ? sphereIdx : cubeIdx,
                .transform     = transform,
                .hMaterial     = hMat,
            });
        }

        /* Example Geometries, Objects and Material.slang */
        {
            std::array<uint32_t, 6> textures;
            textures[0] = mTextureManager->loadTexture("cat_1.png");
            textures[1] = mTextureManager->loadTexture("cat_2.jpg");
            textures[2] = mTextureManager->loadTexture("cat_3.jpg");
            textures[3] = mTextureManager->loadTexture("cat_4.jpg");
            textures[4] = mTextureManager->loadTexture("cat_5.jpg");
            textures[5] = mTextureManager->loadTexture("cat_6.jpg");

            std::array<Handle, 6> catMaterials;
            for (auto i = 0; i < textures.size(); i++)
            {
                catMaterials[i] = mMaterialSystem->acquire({
                    .solidColor  = Random::getColor(),
                    .hTexture    = static_cast<int32_t>(textures[i]),
                });
            }

            for (int32_t i = 0; i < 1024; i++)
            {
                const auto base = Random::get(1.0f, 3.5f);
                auto transform = Transform()
                    .setTranslate(glm::vec3(Random::get(-128, 128), Random::get(-15.0f, 15.0f), Random::get(-128, 128)))
                    .setScale(glm::vec3(base));
                if (i % 2 == 0)
                {
                    addObject<RotatingObject>({
                        .geometryIndex = cubeIdx,
                        .transform     = transform,
                        .hMaterial     = catMaterials[Random::get(0, 5)],
                        .name          = fmt::format("Cube-Obj#{}", i),
                    });
                }
                else
                {
                    const auto hMat = mMaterialSystem->acquire({
                        .solidColor  = Random::getColor(),
                        .pIsEmissive = true,
                    });
                    addObject<Object>({
                        .geometryIndex = sphereIdx,
                        .transform     = transform,
                        .hMaterial     = hMat,
                        .name          = fmt::format("Sphere-Obj#{}", i),
                    });
                }
            }
        }

        #pragma endregion
    }

    void Level::onEvent(const SDL_Event& event) noexcept
    {
        mCameraSystem->onEvent(event);

        if (mSelectObjectFeature)
        {
            mSelectObjectFeature->onEvent(event);
        }
    }

    void Level::onUpdate(const float dt, const RHI::FrameData& frameData, RHI::CommandList* pCommandList) noexcept
    {
        mCameraSystem->onUpdate(frameData);

        mLightSystem->onUpdate(pCommandList);
        mMaterialSystem->onUpdate(pCommandList);

        mGeometrySystem->onUpdate(frameData, pCommandList);
        if (mRHI->getFeatures().rayTracing)
        {
            mBlasSystem->onUpdate(frameData, pCommandList);
        }

        for (const auto& obj : mObjects)
        {
            obj->onUpdate(dt);
            if (obj->isInstanceDirty)
            {
                mInstanceSystem->modify(obj->hInstance, [&](InstanceData& data) -> void {
                    const auto model = obj->getModel();
                    const auto materialIndex = obj->hMaterial.isNull() ? -1 : static_cast<int32_t>(mMaterialSystem->getGpuIndex(obj->hMaterial));
                    const auto bbox = mGeometrySystem->getGeometry(obj->geometryIndex)->getBoundingBox().getTransformed(model);

                    const auto previousModel = obj->isFirstUpdate ? model : data.model;

                    data = {
                        .model          = model,
                        .previousModel  = previousModel,
                        .boundingBox    = bbox,
                        .blas           = mBlasSystem ? mBlasSystem->getGeometryBlasAddress(obj->geometryIndex) : 0,
                        .geometryIndex  = obj->geometryIndex,
                        .materialIndex  = materialIndex,
                        .objectId       = obj->id,
                    };
                });

                obj->isFirstUpdate   = false;
                obj->isInstanceDirty = false;
            }
        }

        mInstanceSystem->onUpdate(pCommandList);

        if (mRHI->getFeatures().rayTracing)
        {
            mTlasSystem->onUpdate(frameData, pCommandList);
        }

        if (!mObjects.empty())
        {
            buildDrawCommands(pCommandList, frameData);
        }
    }

    void Level::onDrawUI()
    {
        mCameraSystem->onDrawUI();
        mLightSystem->onDrawUI();
        if (mSelectObjectFeature)
        {
            mSelectObjectFeature->onDrawUI(mObjects);
        }
    }

    void Level::drawIndexedIndirect(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept
    {
        if (!mObjects.empty())
        {
            commandList->getHandle().drawIndexedIndirect(mDrawCommandsBuffer[frameData.currentFrame]->getHandle(), 0, mDrawCount, sizeof(vk::DrawIndexedIndirectCommand));
        }
    }

    uint64_t Level::getCameraBuffer(const uint32_t frame) const
    {
        return mCameraSystem->getBuffer(frame)->getAddress();
    }

    const std::vector<UPtr<Object>>& Level::getObjects() const noexcept
    {
        return mObjects;
    }

    Object* Level::getSelectedObject() const noexcept
    {
        if (mSelectObjectFeature)
        {
            const auto idx = *mSelectObjectFeature->getSelectedObjectIdx();
            if (idx == -1)
            {
                return nullptr;
            }
            return mObjects[idx].get();
        }
        return nullptr;
    }

    const SPtr<RHI::Buffer>& Level::getInstanceIndirectionBuffer(const uint32_t frameIndex)
    {
        return mInstanceIndirectionMapBuffer[frameIndex];
    }

    void Level::initEmitterData()
    {
        // TODO: For now this is used to initialize emitters before first render, update to be interactive later.

        std::unordered_map<int32_t, uint32_t> geomToCdfOffset;
        uint64_t discretePdfsSize = 0;
        uint32_t elem = 0;
        for (const auto& [geomIndex, dPdf] : mDiscretePDFs)
        {
            geomToCdfOffset.emplace(geomIndex, elem);
            const auto n = dPdf.getValues().size();
            discretePdfsSize += n * sizeof(float);
            elem += static_cast<uint32_t>(n);
        }

        for (auto& e : mEmitters)
        {
            e.cdfOffset = geomToCdfOffset.at(e.geometryIndex);
        }

        const uint64_t emittersSize = sizeof(AreaEmitter) * mEmitters.size();

        const auto uploadBuffer = mRHI->createBuffer({
            .size  = emittersSize + discretePdfsSize,
            .type  = RHI::BufferType::Staging,
            .label = "Level_UploadEmitters",
        });
        uploadBuffer->setData(mEmitters.data(), emittersSize, 0);

        uint64_t offset = emittersSize;
        for (const auto& dPdf : mDiscretePDFs | std::views::values)
        {
            const auto& v = dPdf.getValues();
            const uint64_t size = v.size() * sizeof(float);
            uploadBuffer->setData(v.data(), size, offset);
            offset += size;
        }

        mEmittersBuffer = mRHI->createBuffer({
            .size  = emittersSize,
            .type  = RHI::BufferType::Storage,
            .label = "Level_Emitters",
        });
        mDiscretePDFsBuffer = mRHI->createBuffer({
            .size  = discretePdfsSize,
            .type  = RHI::BufferType::Storage,
            .label = "Level_EmitterDPdfs",
        });

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
        {
            const auto copyEmitters = vk::BufferCopy2 { 0, 0, emittersSize };
            const auto copyBuffer0  = vk::CopyBufferInfo2()
                .setSrcBuffer(uploadBuffer->getHandle())
                .setDstBuffer(mEmittersBuffer->getHandle())
                .setRegions(copyEmitters);

            const auto copyPdfs    = vk::BufferCopy2 { emittersSize, 0, discretePdfsSize };
            const auto copyBuffer1 = vk::CopyBufferInfo2()
                .setSrcBuffer(uploadBuffer->getHandle())
                .setDstBuffer(mDiscretePDFsBuffer->getHandle())
                .setRegions(copyPdfs);

            pCommandList->getHandle().copyBuffer2(copyBuffer0);
            pCommandList->getHandle().copyBuffer2(copyBuffer1);
        });
    }

    void Level::buildDrawCommands(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        for (auto it = mPendingReleases.begin(); it != mPendingReleases.end();)
        {
            if (mRHI->isFrameComplete(it->frameToRelease))
            {
                it = mPendingReleases.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::unordered_map<int32_t, std::vector<Handle>> groups;
        for (const auto& obj : mObjects)
        {
            groups[obj->geometryIndex].push_back(obj->hInstance);
        }

        /**
         * After culling "gl_InstanceIndex" becomes valid for visible instances only, a "dense" index.
         * Due to this we require an indirection map to translate "gl_InstanceIndex" into one
         * that is valid for the InstanceSystem pool.
         */
        std::vector<uint32_t>                       instanceIndirectionMap;
        std::vector<vk::DrawIndexedIndirectCommand> draws;

        auto cullTime = DeltaTime().initialize();

        uint32_t totalInstanceCount        = 0;
        uint32_t totalVisibleInstanceCount = 0;
        for (const auto& [geometryIndex, instanceHandles] : groups)
        {
            const auto firstInstance = static_cast<uint32_t>(instanceIndirectionMap.size());

            uint32_t visibleInstanceCount = 0;
            for (const auto& hInstance : instanceHandles)
            {
                auto shouldKeep = !mEnableCulling;
                if (mEnableCulling)
                {
                    auto* data = mInstanceSystem->get(hInstance);
                    shouldKeep = data->boundingBox.isVisible(mCameraSystem->getActiveCamera()->getFrustumPlanes());
                }
                if (shouldKeep)
                {
                    visibleInstanceCount += 1;
                    instanceIndirectionMap.push_back(mInstanceSystem->getGpuIndex(hInstance));
                }
            }

            totalInstanceCount        += instanceHandles.size();
            totalVisibleInstanceCount += visibleInstanceCount;

            const auto geometryInfo = mGeometrySystem->getGeometryInfo(geometryIndex);
            const auto cmd = vk::DrawIndexedIndirectCommand()
                .setIndexCount(geometryInfo.indexCount)
                .setInstanceCount(visibleInstanceCount)
                .setFirstIndex(geometryInfo.firstIndex)
                .setVertexOffset(static_cast<int32_t>(geometryInfo.firstVertex))
                .setFirstInstance(firstInstance);
            draws.push_back(cmd);
        }

        mLastCullStats = CullStats::make(totalInstanceCount, totalVisibleInstanceCount, cullTime.getDeltaTime());
        mDrawCount     = static_cast<uint32_t>(draws.size());

        // If totalVisibleInstanceCount is 0 keep the result of the last culling to avoid allocating buffers of size 0.
        if (totalVisibleInstanceCount == 0)
        {
            return;
        }

        // Upload data to GPU
        const auto drawSize = mDrawCount * sizeof(vk::DrawIndexedIndirectCommand);

        if (mDrawCommandsBuffer[frameData.currentFrame])
        {
            mPendingReleases.push_back({ mDrawCommandsBuffer[frameData.currentFrame], frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        }
        mDrawCommandsBuffer[frameData.currentFrame] = mRHI->createBuffer({
            .size  = drawSize,
            .type  = RHI::BufferType::Indirect,
            .label = fmt::format("Scene_Draw_Commands_{}", frameData.currentFrame),
        });

        const auto mapSize = instanceIndirectionMap.size() * sizeof(uint32_t);

        if (mInstanceIndirectionMapBuffer[frameData.currentFrame])
        {
            mPendingReleases.push_back({ mInstanceIndirectionMapBuffer[frameData.currentFrame], frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        }
        mInstanceIndirectionMapBuffer[frameData.currentFrame] = mRHI->createBuffer({
            .size  = mapSize,
            .type  = RHI::BufferType::Storage,
            .label = fmt::format("Scene_Instance_Map_{}", frameData.currentFrame),
        });

        if (mBuildDrawCommandsStaging[frameData.currentFrame])
        {
            mPendingReleases.push_back({mBuildDrawCommandsStaging[frameData.currentFrame], frameData.lifetimeFrameCounter + RHI::gFramesInFlight });
        }
        mBuildDrawCommandsStaging[frameData.currentFrame] = mRHI->createBuffer({
            .size  = drawSize + mapSize,
            .type  = RHI::BufferType::Staging,
            .label = "Scene_Staging",
        });
        mBuildDrawCommandsStaging[frameData.currentFrame]->setData(draws.data(), drawSize, 0);
        mBuildDrawCommandsStaging[frameData.currentFrame]->setData(instanceIndirectionMap.data(), mapSize, drawSize);

        const auto drawCopy     = vk::BufferCopy2 { 0, 0, drawSize };
        const auto drawCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mBuildDrawCommandsStaging[frameData.currentFrame]->getHandle())
            .setDstBuffer(mDrawCommandsBuffer[frameData.currentFrame]->getHandle())
            .setRegions(drawCopy);
        pCommandList->getHandle().copyBuffer2(drawCopyInfo);

        const auto mapRegion   = vk::BufferCopy2 { drawSize, 0, mapSize };
        const auto mapCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mBuildDrawCommandsStaging[frameData.currentFrame]->getHandle())
            .setDstBuffer(mInstanceIndirectionMapBuffer[frameData.currentFrame]->getHandle())
            .setRegions(mapRegion);
        pCommandList->getHandle().copyBuffer2(mapCopyInfo);
    }

    void Level::onDrawUI_Culling()
    {
        ImGui::Begin("Culling");

        ImGui::SeparatorText("Options");
        ImGui::Checkbox("Enable Culling", &mEnableCulling);

        ImGui::SeparatorText("Stats");
        ImGui::Text("Object Count: %d", mLastCullStats.totalObjectCount);
        ImGui::Text("Culled Count: %d (%.3f)", mLastCullStats.culledCount, mLastCullStats.percent);
        ImGui::Text("Time: %.3fms", mLastCullStats.cullTimeMs);

        ImGui::End();
    }
}
