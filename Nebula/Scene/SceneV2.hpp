#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "InstancePool.hpp"
#include "LightSystem.hpp"
#include "SceneGeometry.hpp"
#include "TextureManager.hpp"
#include "TLASManager.hpp"
#include "Camera/FlyingCamera.hpp"
#include "Core/Random.hpp"
#include "Core/Ranges.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/Transform.hpp"
#include "Render/FXAAPass.hpp"
#include "Render/LightingPass.hpp"
#include "Render/SSAOPass.hpp"
#include "Scene/Render/Indirect_GBufferPass.hpp"
#include "Voxel/TerrainGenerator.hpp"
#include "Voxel/Features/FoliageGenerator.hpp"
#include "VulkanRHI/Barrier.hpp"

struct Object
{
    virtual      ~Object() = default;
    virtual void onUpdate(float dt) noexcept {}

    // Properties
    SPtr<Geometry> pGeometry    = nullptr;
    int32_t        textureIndex = -1;
    int32_t        geometryIndex = -1;
    int32_t        instanceIndex = -1;
    glm::vec4      solidColor   = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    Transform      transform    = {};

    // Raytracing Properties
    uint32_t        rt_hitGroup  = 0;
    uint32_t        rt_mask      = 0xff;

    // General
    int32_t         id;
    std::string     name;

    [[nodiscard]] GPUInstanceData getInstanceData() noexcept
    {
        return {
            .model         = transform.getModel(),
            .solidColor    = solidColor,
            .textureIndex  = textureIndex,
            .geometryIndex = geometryIndex,
        };
    }
};

struct ExampleObject : public Object
{
    ~ExampleObject() override = default;

    void onUpdate(const float dt) noexcept override
    {
        transform.rotate(glm::vec3(mRotationFactor) * dt);
    }

private:
    const float mRotationFactor = Random::get(2.5f, 15.0f) * (Random::get<int32_t>() % 2 == 0 ? 1.0f : -1.0f);
};

class SceneV2
{
public:
    explicit SceneV2(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
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

        initScene();

        /* TODO: Bindless */ {
            mSceneDescriptor = mRHI->createDescriptor({
                .bindings = {
                    vk::DescriptorSetLayoutBinding {
                        0, vk::DescriptorType::eUniformBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
                    },
                    vk::DescriptorSetLayoutBinding {
                        1, vk::DescriptorType::eStorageBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
                    },
                },
                .setCount = 2,
                .debugName = "Scene_Descriptor",
            });

            for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
            {
                const auto descriptorWrite = RHI::DescriptorWrite()
                    .writeUniformBuffer(0, mCameraUniformBuffers[i])
                    .writeStorageBuffer(1, mLightSystem->getDataBuffer());
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

        mLightingPass = LightingPass::create({
            .resolution = { extent.width, extent.height },
            .input      = { mTestPass->getPosition(), mTestPass->getNormal(), mTestPass->getAlbedo(), mSceneDescriptor, mSSAO->getResult() },
            .rhi        = mRHI,
        });

        mFXAA = FXAAPass::create({
            .resolution = { extent.width, extent.height },
            .input      = { mLightingPass->getResult() },
            .rhi        = mRHI,
        });
    }

    void preFrame() noexcept
    {
        mLightSystem->upload();
    }

    void onEvent(const SDL_Event& event) const noexcept
    {
        if (mCamera)
        {
            mCamera->onEvent(event);
        }
    }

    void onUpdate(const float dt, const RHI::FrameData& frameData, const RHI::CommandList* pCommandList) noexcept
    {
        static bool isFirstUpdate = true;

        // Update Instance Data
        for (const auto& obj : mObjects)
        {
            obj->onUpdate(dt);
            if (obj->transform.isDirty() || isFirstUpdate)
            {
                auto instanceData = obj->getInstanceData();
                if (mRHI->getRaytracingSupport())
                {
                    instanceData.blasAddress = mGeometry->getGeometryBLAS(obj->pGeometry->getName())->getAddress();
                }

                mInstancePool->update(obj->instanceIndex, instanceData);
            }
        }
        mInstancePool->flush(pCommandList);

        // Update Top-Level AS
        if (mRHI->getRaytracingSupport())
        {
            mTLASManager->onUpdate(pCommandList);
        }

        // Update Camera Data
        if (mCamera)
        {
            mCamera->onUpdate();

            const auto cameraData = mCamera->getCameraData();
            mCameraUniformBuffers[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
        }

        // Update draw commands
        if (mObjectCountChanged || isFirstUpdate)
        {
            buildDrawCommands(pCommandList);
        }

        mObjectCountChanged = false;
        isFirstUpdate = false;
    }

    void onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
    {
        mTestPass->execute(commandList, frameData);
        mSSAO->execute(commandList, frameData);
        mLightingPass->execute(commandList, frameData);
        mFXAA->execute(commandList, frameData);

        commandList->beginLabel("Present_Blit");
        // Barriers
        const auto barrier = RHI::Barrier()
            .addBarrier(mFXAA->getResult()->getBarrier(RHI::ImageUsage::TransferSrc))
            .addBarrier(mRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::TransferDst));
        barrier.insert(commandList);

        // Blit
        const auto srcExtent = mFXAA->getResult()->getProperties().extent;
        const auto dstExtent = mRHI->getSwapchain()->getProperties().extent;
        const auto region    = vk::ImageBlit2()
            .setSrcOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(srcExtent.width), static_cast<int32_t>(srcExtent.height), 1 }
            })
            .setSrcSubresource(mFXAA->getResult()->getProperties().getSubresourceLayers())
            .setDstOffsets({
                vk::Offset3D { 0, 0, 0 },
                vk::Offset3D { static_cast<int32_t>(dstExtent.width), static_cast<int32_t>(dstExtent.height), 1 }
            })
            .setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });

        const auto blit = vk::BlitImageInfo2()
            .setSrcImage(mFXAA->getResult()->getImage())
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(mRHI->getSwapchain()->getImage(frameData.acquiredIndex))
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eLinear)
            .setRegions(region);

        commandList->getHandle().blitImage2(blit);
        commandList->endLabel();
    }

    template <class T>
    requires std::is_base_of_v<Object, T>
    void addObject(const SPtr<Geometry>& geometry, const int32_t tex, const Transform transform) noexcept
    {
        auto obj = makeUnique<T>();
        obj->pGeometry = geometry;
        obj->textureIndex = tex;
        obj->transform = transform;
        obj->geometryIndex = mGeometry->getGeometryIndex(geometry->getName());

        auto instanceData = obj->getInstanceData();
        if (mRHI->getRaytracingSupport())
        {
            instanceData.blasAddress = mGeometry->getGeometryBLAS(obj->pGeometry->getName())->getAddress();
        }
        obj->instanceIndex = mInstancePool->acquire(instanceData);

        mObjects.push_back(std::move(obj));

        mObjectCountChanged = true;
    }

    [[nodiscard]] const SPtr<RHI::Descriptor>& getSceneDescriptor() const noexcept
    {
        return mSceneDescriptor;
    }

private:
    void initScene() noexcept
    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mLightSystem->addLight({});

        const auto geoCube = mGeometry->addGeometry<Cube>(Cube::Params {});
        mGeometry->onUpdate();

        const auto geoSphere = mGeometry->addGeometry<Sphere>(Sphere::Params {});
        const auto geoCylinder = mGeometry->addGeometry<Cylinder>(Cylinder::Params {});

        mGeometry->onUpdate();

        mTextureManager->loadTexture("missingTexture.png", 1);
        mTextureManager->loadTexture("missingTexture.png", 2);
        mTextureManager->loadTexture("missingTexture.png", 3);

        addObject<ExampleObject>(geoCube, 1, Transform().translate({ 5.0f, 0.0f, 0.0f }));
        addObject<ExampleObject>(geoSphere, 2, Transform().translate({ 0.0f, 0.0f, 0.0f }));
        addObject<ExampleObject>(geoCylinder, 3, Transform().translate({ -5.0f, 0.0f, 0.0f }));

        for (uint32_t i = 0; i < 256; i++)
        {
            auto geometry = geoCube;
            if (Random::get(0, 128) % 2 == 0)
            {
                geometry = geoSphere;
            }
            if (Random::get(0, 128) % 3 == 0)
            {
                geometry = geoCylinder;
            }

            const auto transform = Transform()
                .translate({
                    Random::get(-64.0f, 64.0f),
                    Random::get(-64.0f, 64.0f),
                    Random::get(-64.0f, 64.0f),
                });
            addObject<ExampleObject>(geometry, 1, transform);
            mObjects.back()->solidColor = Random::getColor();
        }

        auto terrainGenerator = vxl::TerrainGenerator({ 256, 24, 96, true });
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

        for (const auto& voxel : terrainGenerator.getResult())
        {
            auto t = Transform().setScale(voxel.scale).setTranslate(voxel.position);
            addObject<Object>(geoCube, 1, t);
            mObjects.back()->solidColor = glm::vec4(voxel.color, 1.0f);
        }
    }

    void buildDrawCommands(const RHI::CommandList* pCommandList) noexcept
    {
        std::unordered_map<Geometry*, std::vector<uint32_t>> groups;
        for (const auto& obj : mObjects)
        {
            groups[obj->pGeometry.get()].push_back(obj->instanceIndex);
        }

        std::vector<uint32_t> instanceMap;
        std::vector<vk::DrawIndexedIndirectCommand> draws;
        for (auto& [geometry, instanceIndices] : groups)
        {
            auto& info = mGeometry->getGeometryInfo(geometry->getName());

            const auto firstInstance = static_cast<uint32_t>(instanceMap.size());
            for (auto instanceIndex : instanceIndices)
            {
                instanceMap.push_back(instanceIndex);
            }

            const auto cmd = vk::DrawIndexedIndirectCommand()
                .setIndexCount(info.indexRegion.indexCount)
                .setInstanceCount(instanceIndices.size())
                .setFirstIndex(info.indexRegion.firstIndex)
                .setVertexOffset(static_cast<int32_t>(info.vertexRegion.firstVertex))
                .setFirstInstance(firstInstance);
            draws.push_back(cmd);
        }

        mDrawCount = mGeometry->getCount();

        const auto drawSize = mDrawCount * sizeof(vk::DrawIndexedIndirectCommand);
        mDrawCmdBuffer = mRHI->createBuffer({
            .size  = drawSize,
            .type  = RHI::BufferType::Indirect,
            .label = "Scene_Draw_Commands",
        });

        const auto mapSize = instanceMap.size() * sizeof(uint32_t);
        mInstanceMapBuffer = mRHI->createBuffer({
            .size  = mapSize,
            .type  = RHI::BufferType::Storage,
            .label = "Scene_Instance_Map"
        });

        mDrawStaging = mRHI->createBuffer({
            .size  = drawSize + mapSize,
            .type  = RHI::BufferType::Staging,
            .label = "Scene_Staging",
        });
        mDrawStaging->setData(draws.data(), drawSize, 0);
        mDrawStaging->setData(instanceMap.data(), mapSize, drawSize);

        const auto drawCopy = vk::BufferCopy2()
            .setSrcOffset(0)
            .setDstOffset(0)
            .setSize(drawSize);
        const auto drawCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mDrawStaging->getHandle())
            .setDstBuffer(mDrawCmdBuffer->getHandle())
            .setRegions(drawCopy);
        pCommandList->getHandle().copyBuffer2(drawCopyInfo);

        const auto mapCopy = vk::BufferCopy2()
            .setSrcOffset(drawSize)
            .setDstOffset(0)
            .setSize(mapSize);
        const auto mapCopyInfo = vk::CopyBufferInfo2()
            .setSrcBuffer(mDrawStaging->getHandle())
            .setDstBuffer(mInstanceMapBuffer->getHandle())
            .setRegions(mapCopy);
        pCommandList->getHandle().copyBuffer2(mapCopyInfo);
    }

    friend class Indirect_GBufferPass;

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<SceneGeometry>                 mGeometry;
    UPtr<InstancePool>                  mInstancePool;
    UPtr<TextureManager>                mTextureManager;
    UPtr<TLASManager>                   mTLASManager;

    UPtr<LightSystem>                   mLightSystem;

    UPtr<ICamera>                       mCamera;
    PerFrameArray<SPtr<RHI::Buffer>>    mCameraUniformBuffers;

    SPtr<RHI::Descriptor>               mSceneDescriptor;

    std::vector<UPtr<Object>>           mObjects;
    bool                                mObjectCountChanged = false;

    SPtr<RHI::Buffer>                   mDrawStaging;
    SPtr<RHI::Buffer>                   mDrawCmdBuffer;
    SPtr<RHI::Buffer>                   mInstanceMapBuffer;
    uint32_t                            mDrawCount = 0;

    UPtr<Indirect_GBufferPass>          mTestPass;
    UPtr<SSAOPass>                      mSSAO;
    UPtr<LightingPass>                  mLightingPass;
    UPtr<FXAAPass>                      mFXAA;
};
