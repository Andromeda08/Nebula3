#include "PathTracerView.hpp"

#include <imgui.h>
#include <glm/gtx/hash.hpp>

#include "Core/Random.hpp"
#include "Level/Camera/FlyingCamera.hpp"
#include "Level/Camera/OrbitCamera.hpp"
#include "Level/Voxel/TerrainGenerator.hpp"



namespace nbl
{
    PTScene::PTScene(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager)
    : mRHI(rhi)
    {
        mCameraSystem = makeUnique<CameraSystem>(rhi);

        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCameraSystem->addCamera<FlyingCamera>(false, glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));
        mCameraSystem->addCamera<OrbitCamera>(true);

        mGeometrySystem = makeUnique<GeometrySystem>(mRHI);
        mLightSystem    = makeUnique<LightSystem>(mRHI);
        mMaterialSystem = makeUnique<MaterialSystem>(mRHI, 65536);
        mInstanceSystem = makeUnique<InstanceSystem>(mRHI, 4194304);
        mBlasSystem     = makeUnique<BLASSystem>(mRHI, mGeometrySystem.get());
        mTlasSystem     = makeUnique<TLASSystem>(mRHI, mInstanceSystem.get());

        mSelectObjectFeature = makeUnique<SelectObjectFeature>(mRHI, mCameraSystem.get(), mInstanceSystem.get(), mTlasSystem.get());

        const int32_t cubeIdx   = mGeometrySystem->addGeometry(Cube::createGeometry());
        const int32_t sphereIdx = mGeometrySystem->addGeometry(Sphere::createGeometry());

        auto terrainGenerator = vxl::TerrainGenerator({ 256, 24, 128, true });
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
                .bsdfIndex  = Random::get<uint32_t>(1, 2),
            });

            auto transform = Transform()
                .setTranslate(glm::vec3(Random::get(-128, 128), Random::get(-32.0f, 32.0f), Random::get(-128, 128)));

            addObject({
                .geometryIndex = Random::get<int32_t>() % 2 == 0 ? sphereIdx : cubeIdx,
                .transform     = transform,
                .hMaterial     = hMat,
            });
        }

        addObject({
            .geometryIndex = cubeIdx,
            .transform     = Transform().setTranslate({96.0f, 48.0f, 0.0f}).setRotation({ 45.0f, 90.0f, 0.0f }).setScale({ 64.0f, 0.1f, 36.0f }),
            // TODO: Add support for null materials
            .hMaterial     = mMaterialSystem->acquire({}),
            .isEmitter     = true,
            .radiance      = glm::vec3(50.0f),
        });

        addObject({
            .geometryIndex = cubeIdx,
            .transform     = Transform().setTranslate({ 128.0f, 0.0f, 0.0f}).setRotation({ 0.0f, 0.0f, 90.0f }).setScale({ 256.0f, 0.1f, 256.0f}),
            .hMaterial     = mMaterialSystem->acquire({
                .solidColor = { 0.027f, 0.0f, 0.376f, 1.0f },
            }),
        });

        addObject({
            .geometryIndex = cubeIdx,
            .transform     = Transform().setTranslate({ 0.0f, 0.0f, 128.0f}).setRotation({ 0.0f, 90.0f, 90.0f }).setScale({ 256.0f, 0.1f, 256.0f}),
            .hMaterial     = mMaterialSystem->acquire({
                .solidColor = { 0.027f, 0.0f, 0.376f, 1.0f },
            }),
        });

        addObject({
            .geometryIndex = cubeIdx,
            .transform     = Transform().setTranslate({ 0.0f, 0.0f, -128.0f}).setRotation({ 0.0f, 90.0f, 90.0f }).setScale({ 256.0f, 0.1f, 256.0f}),
            .hMaterial     = mMaterialSystem->acquire({
                .solidColor = { 0.027f, 0.0f, 0.376f, 1.0f },
            }),
        });

        addObject({
            .geometryIndex = cubeIdx,
            .transform     = Transform().setTranslate({ 0.0f, 128.0f, 0.0f}).setRotation({ 0.0f, 0.0, 0.0f }).setScale({ 256.0f, 0.1f, 256.0f}),
            .hMaterial     = mMaterialSystem->acquire({
                .solidColor = { 0.027f, 0.0f, 0.376f, 1.0f },
            }),
        });

        initEmitterData();
    }

    Object* PTScene::addObject(const PTObjectParams& params)
    {
        mObjects.push_back(makeUnique<Object>());
        auto* obj = mObjects.back().get();

        obj->id              = static_cast<int32_t>(mObjects.size()) - 1;
        obj->name            = params.name.value_or(fmt::format("Object #{}", mObjects.size()));
        obj->geometryIndex   = params.geometryIndex;
        obj->transform       = params.transform;
        obj->hMaterial       = params.hMaterial;
        obj->blasAddress     = 0;
        obj->isInstanceDirty = true;
        obj->hInstance       = mInstanceSystem->acquire({});

        if (params.isEmitter)
        {
            const auto* geometry = mGeometrySystem->getGeometry(obj->geometryIndex);

            if (!mDiscretePDFs.contains(obj->geometryIndex))
            {
                DiscretePDF discretePdf;
                discretePdf.clear();
                discretePdf.reserve(geometry->getTriangleCount());

                for (size_t i = 0; i < geometry->getIndexCount(); i += 3)
                {
                    // TODO: Functions getTriangle and getTriangleArea for Geometry.
                    const size_t    i0 = geometry->getIndices().at(i + 0);
                    const size_t    i1 = geometry->getIndices().at(i + 1);
                    const size_t    i2 = geometry->getIndices().at(i + 2);
                    const glm::vec3 v0 = geometry->getVertices().at(i0).position;
                    const glm::vec3 v1 = geometry->getVertices().at(i1).position;
                    const glm::vec3 v2 = geometry->getVertices().at(i2).position;

                    const float area = 0.5f * glm::length(glm::cross(v1 - v0, v2 - v0));
                    discretePdf.append(area);
                }

                discretePdf.normalize();

                mDiscretePDFs[obj->geometryIndex] = std::move(discretePdf);
            }

            const AreaEmitter emitterInfo = {
                .instanceIndex = mInstanceSystem->getGpuIndex(obj->hInstance),
                .geometryIndex = obj->geometryIndex,
                .cdfOffset     = std::numeric_limits<uint32_t>::max(),
                .triCount      = geometry->getTriangleCount(),
                .totalWeight   = mDiscretePDFs[obj->geometryIndex].getSum(),
                .radiance      = params.radiance.value_or(glm::vec3(1.0f)),
            };

            mEmitters.push_back(emitterInfo);

            obj->emitterIndex = mEmitters.size() - 1;
        }

        return obj;
    }

    void PTScene::onEvent(const SDL_Event& event) const noexcept
    {
        mCameraSystem->onEvent(event);
        mSelectObjectFeature->onEvent(event);
    }

    void PTScene::onUpdate(float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) const noexcept
    {
        mCameraSystem->onUpdate(frameData);
        mLightSystem->onUpdate(pCommandList);
        mMaterialSystem->onUpdate(pCommandList);
        mGeometrySystem->onUpdate(frameData, pCommandList);
        mBlasSystem->onUpdate(frameData, pCommandList);

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
                        .emitterIndex   = obj->emitterIndex,
                    };
                });

                obj->isFirstUpdate   = false;
                obj->isInstanceDirty = false;
            }
        }

        mInstanceSystem->onUpdate(pCommandList);

        mTlasSystem->onUpdate(frameData, pCommandList);
    }

    void PTScene::initEmitterData()
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
            .label = "PTScene_UploadEmitters",
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
            .label = "Emitters",
        });
        mDiscretePDFsBuffer = mRHI->createBuffer({
            .size  = discretePdfsSize,
            .type  = RHI::BufferType::Storage,
            .label = "EmitterDPdfs",
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

    void PathTracerView::onEvent(const SDL_Event& event)
    {
        mScene->onEvent(event);
    }

    void PathTracerView::onUpdate(const float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        mScene->onUpdate(dt, frameData, pCommandList);

        const auto cameraView = mScene->mCameraSystem->getActiveCamera()->getGpuCameraData().view;

        bool viewChanged = false;
        for (int32_t i = 0; i < 4 && !viewChanged; ++i)
        {
            if (glm::any(glm::epsilonNotEqual(cameraView[i], mPrevView[i], 1e-6f)))
            {
                viewChanged = true;
            }
        }

        if (viewChanged)
        {
            mAccumStartTime    = std::chrono::high_resolution_clock::now();
            mAccumulatedFrames = 0;
        }
        else
        {
            if (mMaximumSamples != 0 && (mAccumulatedFrames >= mMaximumSamples))
            {
                mAccumEndTime = std::chrono::high_resolution_clock::now();
            }
            mAccumulatedFrames += 1;
        }

        mTotalFrames++;
        mPrevView = cameraView;
    }

    void PathTracerView::onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        pCommandList->beginLabel("PathTracerView::onRender()");

        PathTracerPushConstants pushConstants = {
            .camera         = mScene->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .instances      = mScene->mInstanceSystem->getBuffer()->getAddress(),
            .emitters       = mScene->mEmittersBuffer->getAddress(),
            .emitterPdfs    = mScene->mDiscretePDFsBuffer->getAddress(),
            .vertices       = mScene->mGeometrySystem->getBuffers().getVertexBuffer()->getAddress(),
            .indices        = mScene->mGeometrySystem->getBuffers().getIndexBuffer()->getAddress(),
            .materials      = mScene->mMaterialSystem->getBuffer()->getAddress(),
            .geometryInfos  = mScene->mGeometrySystem->getBuffers().getInfoBuffer()->getAddress(),
            .accumulated    = mAccumulatedFrames,
            .totalFrames    = mTotalFrames,
            .maxBounces     = static_cast<uint32_t>(mMaxBounces),
            .spp            = static_cast<uint32_t>(mSPP),
            .bDynamicRR     = mDynamicRR ? 1 : 0,
            .rrCont         = mRRCont,
            .emitterCount   = static_cast<uint32_t>(mScene->mEmitters.size())
        };

        const bool isMaxSamples = mMaximumSamples != 0 && (mAccumulatedFrames >= mMaximumSamples);
        if (!isMaxSamples)
        {
            RHI::Barrier()
                .addBarrier(mCurrentOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(mAccumulatedOutput[0]->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(mAccumulatedOutput[1]->getBarrier(RHI::ImageUsage::StorageImage))
                .addBarrier(mScene->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
                .addBarrier(mScene->mTlasSystem->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
                .insert(pCommandList);

            pCommandList->bindPipeline(mPipeline.get());
            pCommandList->bindDescriptorSets({
                mDescriptor->getSet(frameData.currentFrame),
                mScene->mTlasSystem->getDescriptor()->getSet(frameData.currentFrame),
            });
            pCommandList->pushConstants(&pushConstants);

            const auto [w, h] = mCurrentOutput[frameData.currentFrame]->getProperties().extent;
            pCommandList->traceRays(w, h, 1);
        }

        const uint32_t writeAccum = (frameData.currentFrame == 0) ? 1 : 0;

        RHI::Barrier()
            .addBarrier(mAccumulatedOutput[writeAccum]->getBarrier(RHI::ImageUsage::StorageImage))
            .insert(pCommandList);

        mTonemapPass->execute(mAccumulatedOutput[writeAccum], pCommandList, frameData);
        // mFXAAPass->execute(pCommandList, frameData);

        pCommandList->blitToSwapchain(mTonemapPass->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);

        pCommandList->endLabel();
    }

    void PathTracerView::onDrawUI()
    {
        const bool reachedMax = mMaximumSamples != 0 && (mAccumulatedFrames >= mMaximumSamples);
        const auto endpoint   = reachedMax ? mAccumEndTime : std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> delta = endpoint - mAccumStartTime;
        const float dt = delta.count();

        ImGui::Begin("PathTracer");

        ImGui::Text("Samples: %llu", std::min(mAccumulatedFrames, static_cast<uint64_t>(mMaximumSamples)));
        ImGui::Text("Time: %fs", dt);

        ImGui::SeparatorText("Params");

        bool changed = false;

        changed |= ImGui::DragInt("SPP", &mSPP, 1, 0, 64);
        changed |= ImGui::DragInt("Max Bounces", &mMaxBounces, 1, 0, 32);
        changed |= ImGui::DragInt("Max Samples", &mMaximumSamples, 32, 0, 131072);

        changed |= ImGui::Checkbox("Dynamic RR", &mDynamicRR);
        changed |= ImGui::DragFloat("RR Cont", &mRRCont, 0.01f, 0.0f, 1.0f);

        ImGui::End();

        if (changed)
        {
            mPrevView = glm::mat4(1.0f);

        }
    }
}
