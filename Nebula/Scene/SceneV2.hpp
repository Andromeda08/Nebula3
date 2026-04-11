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
#include "Math/BoundingBox.hpp"
#include "Math/Transform.hpp"
#include "Render/AABBOverlayPass.hpp"
#include "Render/FullRT.hpp"
#include "Render/FXAAPass.hpp"
#include "Render/LightingPass.hpp"
#include "Render/ProceduralSky.hpp"
#include "Render/RTAOPass.hpp"
#include "Render/SSAOPass.hpp"
#include "Render/TonemapPass.hpp"
#include "Scene/Render/Indirect_GBufferPass.hpp"
#include "UserInterface/UserInterface.hpp"
#include "Voxel/TerrainGenerator.hpp"
#include "Voxel/Features/FoliageGenerator.hpp"
#include "VulkanRHI/Barrier.hpp"

struct Object
{
    virtual      ~Object() = default;
    virtual void onUpdate(float dt) noexcept {}

    // Properties
    GeometryView   geometry;
    int32_t        textureIndex = -1;
    int32_t        normalIndex   = -1;
    int32_t        instanceIndex = -1;
    glm::vec4      solidColor   = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    Transform      transform    = {};

    // AABB
    glm::vec4      min;
    glm::vec4      max;

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
            .geometryIndex = geometry.metadata->index,
            .blasAddress   = geometry.metadata->blasAddress,
            .normalIndex   = normalIndex,
            ._p0           = 0,
            ._p1           = 0,
            ._p2           = 0,
            .min           = min,
            .max           = max,
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
    explicit SceneV2(const SPtr<RHI::VulkanRHI>& rhi, UserInterface* pUI);

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
        if (mObjectCountChanged || isFirstUpdate)
        {
            buildDrawCommands(pCommandList);
        }

        mObjectCountChanged = false;
        isFirstUpdate = false;
    }

    void onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const noexcept;

    const std::vector<UPtr<Object>>& getObjects() const noexcept { return mObjects; }

    template <class T>
    requires std::is_base_of_v<Object, T>
    void addObject(const GeometryIndex geometryIndex, const int32_t tex, const Transform transform, const glm::vec4& min, const glm::vec4& max, const int32_t normalTex = -1) noexcept
    {
        auto obj = makeUnique<T>();
        obj->geometry      = mGeometry->getGeometryView(geometryIndex);
        obj->transform     = transform;
        obj->textureIndex  = tex;
        obj->normalIndex   = normalTex;

        const auto model = obj->transform.getModel();
        glm::vec4 worldMin = model[3];
        glm::vec4 worldMax = model[3];
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                float a = model[j][i] * min[j];
                float b = model[j][i] * max[j];
                worldMin[i] += glm::min(a, b);
                worldMax[i] += glm::max(a, b);
            }
        }
        obj->min = worldMin;
        obj->max = worldMax;

        obj->instanceIndex = mInstancePool->acquire(obj->getInstanceData());

        mObjects.push_back(std::move(obj));

        mObjectCountChanged = true;
    }

    [[nodiscard]] const SPtr<RHI::Descriptor>& getSceneDescriptor() const noexcept
    {
        return mSceneDescriptor;
    }

    [[nodiscard]] uint64_t getCurrentCameraBufferAddress(const uint32_t i) const
    {
        return mCameraUniformBuffers[i]->getAddress();
    }

private:
    void initScene() noexcept
    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

        mLightSystem->addLight({});
        mLightSystem->addLight({
            .vector      = { -45.0f, 50.0f, -50.0f },
            .color       = { 185.0f / 255.0f, 173.0f / 255.0f, 93.0f / 255.0f },
            .intensity   = 1500.0f,
            .isEnabled   = true,
            .castsShadow = true,
            .radius      = 10.0f,
            .type        = LightType::Point,
        });
        mLightSystem->addLight({
            .vector      = { -17, 50, 35 },
            .color       = { 23.0f / 255.0f, 173.0f / 255.0f, 93.0f / 255.0f },
            .intensity   = 1500.0f,
            .isEnabled   = true,
            .castsShadow = true,
            .radius      = 10.0f,
            .type        = LightType::Point,
        });

        const auto geoCube = mGeometry->addGeometry<Cube>(Cube::Params {});
        mGeometry->commit();

        const auto geoSphere = mGeometry->addGeometry<Sphere>(Sphere::Params {});
        const auto geoCylinder = mGeometry->addGeometry<Cylinder>(Cylinder::Params {});

        mGeometry->commit();

        mTextureManager->loadTexture("missingTexture.png", 1);
        mTextureManager->loadTexture("missingTexture.png", 2);
        mTextureManager->loadTexture("missingTexture.png", 3);

        addObject<ExampleObject>(geoCube, 1, Transform().translate({ 5.0f, 0.0f, 0.0f }), glm::vec4(0.0f), glm::vec4(0.0f));
        addObject<ExampleObject>(geoSphere, 2, Transform().translate({ 0.0f, 0.0f, 0.0f }), glm::vec4(0.0f), glm::vec4(0.0f));
        addObject<ExampleObject>(geoCylinder, 3, Transform().translate({ -5.0f, 0.0f, 0.0f }), glm::vec4(0.0f), glm::vec4(0.0f));

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
            addObject<ExampleObject>(geometry, 1, transform, glm::vec4(0.0f), glm::vec4(0.0f));
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
            addObject<Object>(geoCube, 1, t, glm::vec4(0.0f), glm::vec4(0.0f));
            mObjects.back()->solidColor = glm::vec4(voxel.color, 1.0f);
        }
    }

    void buildDrawCommands(const RHI::CommandList* pCommandList) noexcept
    {
        std::unordered_map<GeometryIndex, std::vector<uint32_t>> groups;
        for (const auto& obj : mObjects)
        {
            groups[obj->geometry.metadata->index].push_back(obj->instanceIndex);
        }

        // Frustum Cull
        #pragma region
        const auto cameraData = mCamera->getCameraData();
        const auto vp = cameraData.proj * cameraData.view;
        const auto vpt = glm::transpose(vp);

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

        uint32_t totalInstanceCount = 0;
        uint32_t totalVisibleInstanceCount = 0;

        std::vector<uint32_t> instanceMap;
        std::vector<vk::DrawIndexedIndirectCommand> draws;
        for (auto& [geometryIndex, instanceIndices] : groups)
        {
            // Redundant query, fix later, obj already has this data
            auto geometryView = mGeometry->getGeometryView(geometryIndex);

            const auto firstInstance = static_cast<uint32_t>(instanceMap.size());

            uint32_t visibleInstanceCount = 0;
            for (auto instanceIndex : instanceIndices)
            {
                const auto instanceData = mInstancePool->getData().at(instanceIndex);
                if (BoundingBox::isVisible(instanceData.min, instanceData.max, frustumPlanes))
                {
                    visibleInstanceCount += 1;
                    instanceMap.push_back(instanceIndex);
                }
            }

            totalInstanceCount += instanceIndices.size();
            totalVisibleInstanceCount += visibleInstanceCount;

            const auto cmd = vk::DrawIndexedIndirectCommand()
                .setIndexCount(geometryView.metadata->indexCount)
                .setInstanceCount(visibleInstanceCount)
                .setFirstIndex(geometryView.metadata->firstIndex)
                .setVertexOffset(static_cast<int32_t>(geometryView.metadata->firstVertex))
                .setFirstInstance(firstInstance);
            draws.push_back(cmd);
        }

        mDrawCount = mGeometry->getGeometryCount();

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

        spdlog::debug("Visible instances: {}/{} (culled={})", totalVisibleInstanceCount, totalInstanceCount, totalInstanceCount - totalVisibleInstanceCount);
    }

    friend class Indirect_GBufferPass;
    friend class SceneInfoComponent;

    SPtr<RHI::VulkanRHI>                mRHI;
    UserInterface*                      mUserInterface;

    // Vertex, Index, BLAS and GeometryInfo buffers
    // Referenced by (via Index):
    // - Objects
    // - InstancePool data
    // ============================================================
    UPtr<SceneGeometry>                 mGeometry;
    UPtr<InstancePool>                  mInstancePool;

    UPtr<TLASManager>                   mTLASManager;

    UPtr<TextureManager>                mTextureManager;

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

    UPtr<Indirect_GBufferPass>          mGBufferPass;
    UPtr<SSAOPass>                      mSSAO;
    UPtr<RTAOPass>                      mRTAO;
    UPtr<ProceduralSkyPass>             mProcSky;
    UPtr<LightingPass>                  mLightingPass;
    UPtr<FXAAPass>                      mFXAA;
    UPtr<TonemapPass>                   mTonemapPass;
    UPtr<AABBOverlayPass>               mAABBPass;

    UPtr<FullRTPass>                    mRTPass;

    std::string                         mName = "Scene V2 Test";
};
