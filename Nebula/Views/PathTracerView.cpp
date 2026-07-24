#include "PathTracerView.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include "Core/Random.hpp"
#include "Level/Camera/FlyingCamera.hpp"
#include "Level/Voxel/TerrainGenerator.hpp"

namespace nbl
{
    PTScene::PTScene(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager)
    : mRHI(rhi)
    {
        mCameraSystem = makeUnique<CameraSystem>(rhi);

        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCameraSystem->addCamera<FlyingCamera>(false, glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mGeometrySystem = makeUnique<GeometrySystem>(mRHI);
        mLightSystem    = makeUnique<LightSystem>(mRHI);
        mMaterialSystem = makeUnique<MaterialSystem>(mRHI, 65536);
        mInstanceSystem = makeUnique<InstanceSystem>(mRHI, 4194304);
        mBlasSystem     = makeUnique<BLASSystem>(mRHI, mGeometrySystem.get());
        mTlasSystem     = makeUnique<TLASSystem>(mRHI, mInstanceSystem.get());

        mSelectObjectFeature = makeUnique<SelectObjectFeature>(mRHI, mCameraSystem.get(), mInstanceSystem.get(), mTlasSystem.get(), nullptr);

        #pragma region "Generate Test Scene"

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

        #pragma endregion

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
                mDiscretePDFs[obj->geometryIndex] = DiscretePDF(mGeometrySystem->getGeometry(obj->geometryIndex));
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

    void PTScene::onDrawUI()
    {
        mSelectObjectFeature->onDrawUI(mObjects);
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

    PathTracerView::PathTracerView(nbl_ViewCtorParams): nbl_ViewBaseCtor
    {
        const bool hasDLSS = mRHI->getDLSS()->isAvailable();
        const bool hasRT   = mRHI->getFeatures().rayTracing;
        if (!hasDLSS || !hasRT)
        {
            exitWithError("Failed to create PathTracerView: Feature requirements were not met\n\t- Ray Tracing: {}\n\t- DLSS Ray Reconstruction: {}",
                hasRT ? "Yes" : "No", hasDLSS ? "Yes" : "No");
        }

        mRHI->getDLSS()->getDLSSDenoiserFeature(RHI::Integration::DLSS_DenoiserParams
        {
            .inExtent     = mRHI->getSwapchain()->getProperties().extent,
            .targetExtent = mRHI->getSwapchain()->getProperties().extent,
        });

        using enum vk::ShaderStageFlagBits;

        mName  = "PathTracerView";
        mScene = makeUnique<PTScene>(mRHI, mTextureManager);

        mDescriptor = mRHI->createDescriptor({
            .bindings  = {
                { 0, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 1, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 2, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 3, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 4, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 5, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 6, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
                { 7, vk::DescriptorType::eStorageImage, 1, eRaygenKHR },
            },
            .setCount  = RHI::gFramesInFlight,
            .debugName = "PT_Descriptor",
        });

        for (uint32_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            mCurrentOutput[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_1spp_   {}",    i), vk::Format::eR32G32B32A32Sfloat);
            mNormals[i]         = makeRenderTarget(mRHI.get(), fmt::format("PT_Normals_{}",    i));
            mDiffuseAlbedo[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_DiffAlbedo_{}", i));
            mSpecularAlbedo[i]  = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecAlbedo_{}", i));
            mRoughness[i]       = makeRenderTarget(mRHI.get(), fmt::format("PT_Roughness_{}",  i), vk::Format::eR16Sfloat);
            mDepth[i]           = makeRenderTarget(mRHI.get(), fmt::format("PT_LinDepth_{}",   i), vk::Format::eR32Sfloat);
            mMotionVectors[i]   = makeRenderTarget(mRHI.get(), fmt::format("PT_MVec_{}",       i), vk::Format::eR32G32Sfloat);
            mRROutput[i]        = makeRenderTarget(mRHI.get(), fmt::format("PT_DLSS_RR_{}",    i));
            mSpecularHitDist[i] = makeRenderTarget(mRHI.get(), fmt::format("PT_SpecHitDist_{}",i), vk::Format::eR32Sfloat);

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
            .addDescriptorLayout(1, mScene->mTlasSystem->getDescriptor().get())
            .setPushConstant<PathTracerPushConstants>(eRaygenKHR | eClosestHitKHR | eMissKHR | eCallableKHR);
        mPipeline = mRHI->createRayTracingPipeline2(ps, common);

        mTonemapPass = makeUnique<TonemapPass>(Tonemap_Params {
            .outputFormat = vk::Format::eR32G32B32A32Sfloat,
            .rhi          = mRHI,
        });
    }

    void PathTracerView::onEvent(const SDL_Event& event)
    {
        mScene->onEvent(event);
    }

    void PathTracerView::onUpdate(const float dt, RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        mScene->onUpdate(dt, frameData, pCommandList);

        mTotalFrames++;

        const uint32_t phase = static_cast<uint32_t>(mTotalFrames % 8) + 1;
        mJitterX = halton(phase, 2) - 0.5f;
        mJitterY = halton(phase, 3) - 0.5f;
    }

    void PathTracerView::onRender(RHI::CommandList* pCommandList, const RHI::FrameData& frameData)
    {
        pCommandList->beginLabel("PathTracerView::onRender()");

        const PathTracerPushConstants pushConstants = {
            .camera         = mScene->mCameraSystem->getBuffer(frameData.currentFrame)->getAddress(),
            .prevCamera     = mScene->mCameraSystem->getPreviousBuffer(frameData.currentFrame)->getAddress(),
            .instances      = mScene->mInstanceSystem->getBuffer()->getAddress(),
            .emitters       = mScene->mEmittersBuffer->getAddress(),
            .emitterPdfs    = mScene->mDiscretePDFsBuffer->getAddress(),
            .vertices       = mScene->mGeometrySystem->getBuffers().getVertexBuffer()->getAddress(),
            .indices        = mScene->mGeometrySystem->getBuffers().getIndexBuffer()->getAddress(),
            .materials      = mScene->mMaterialSystem->getBuffer()->getAddress(),
            .geometryInfos  = mScene->mGeometrySystem->getBuffers().getInfoBuffer()->getAddress(),
            .accumulated    = mTotalFrames, // Not needed for the DLSS Version
            .totalFrames    = mTotalFrames,
            .maxBounces     = 4,
            .spp            = 1,
            .bDynamicRR     = 0,
            .rrCont         = 0.7f,
            .emitterCount   = static_cast<uint32_t>(mScene->mEmitters.size()),
            .jitterX        = mJitterX,
            .jitterY        = mJitterY,
        };

        RHI::Barrier()
            .addBarrier(mCurrentOutput[frameData.currentFrame]->getBarrier(RHI::ImageUsage::StorageImage))
            .addBarrier(mScene->mInstanceSystem->getBuffer()->getBarrier(RHI::BufferUsage::Compute_Read, RHI::BufferUsage::StorageRead))
            .addBarrier(mScene->mTlasSystem->getBackingBuffer()->getBarrier(RHI::BufferUsage::AS_BuildUpdate, RHI::BufferUsage::AS_Traverse))
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
            mScene->mTlasSystem->getDescriptor()->getSet(frameData.currentFrame),
        });
        pCommandList->pushConstants(&pushConstants);

        const auto [w, h] = mCurrentOutput[frameData.currentFrame]->getProperties().extent;
        pCommandList->traceRays(w, h, 1);

        execute_DLSSDenoiser(pCommandList, frameData);
        mTonemapPass->execute(mRROutput[frameData.currentFrame], pCommandList, frameData);

        pCommandList->blitToSwapchain(mTonemapPass->getResult(frameData.currentFrame).get(), mRHI->getSwapchain(), frameData.acquiredIndex);

        pCommandList->endLabel();
    }

    void PathTracerView::onDrawUI()
    {
    }

    void PathTracerView::execute_DLSSDenoiser(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const
    {
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

        using RHI::Integration::DLSS;
        auto resColor       = DLSS::wrapImage(mCurrentOutput[frameData.currentFrame], false);
        auto resNormal      = DLSS::wrapImage(mNormals[frameData.currentFrame], false);
        auto resDiffAlb     = DLSS::wrapImage(mDiffuseAlbedo[frameData.currentFrame], false);
        auto resSpecAlb     = DLSS::wrapImage(mSpecularAlbedo[frameData.currentFrame], false);
        auto resRough       = DLSS::wrapImage(mRoughness[frameData.currentFrame], false);
        auto resDepth       = DLSS::wrapImage(mDepth[frameData.currentFrame], false);
        auto resMotion      = DLSS::wrapImage(mMotionVectors[frameData.currentFrame], false);
        auto resSpecHitDist = DLSS::wrapImage(mSpecularHitDist[frameData.currentFrame], false);
        auto resOutput      = DLSS::wrapImage(mRROutput[frameData.currentFrame], true);

        const auto& cam = mScene->mCameraSystem->getActiveCamera()->getGpuCameraData();
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

        mRHI->getDLSS()->evalDLSSDenoiser(pCommandList, &eval);
    }

    float PathTracerView::halton(uint32_t index, const uint32_t base)
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
