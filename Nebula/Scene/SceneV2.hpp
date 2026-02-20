#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "InstancePool.hpp"
#include "LightSystem.hpp"
#include "SceneGeometry.hpp"
#include "TLASManager.hpp"
#include "Camera/FlyingCamera.hpp"
#include "Core/Random.hpp"
#include "Core/Ranges.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/Transform.hpp"
#include "Voxel/TerrainGenerator.hpp"
#include "Voxel/Features/FoliageGenerator.hpp"

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

        mTLASManager = TLASManager::create({ mRHI, mInstancePool.get() });

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
        for (const auto& obj : mObjects)
        {
            obj->onUpdate(dt);
            if (obj->transform.isDirty() || isFirstUpdate)
            {
                auto instanceData = obj->getInstanceData();
                instanceData.blasAddress = mGeometry->getGeometryBLAS(obj->pGeometry->getName())->getAddress();

                mInstancePool->update(obj->instanceIndex, instanceData);
            }
        }
        mInstancePool->flush(pCommandList);
        mTLASManager->onUpdate(pCommandList);

        if (mCamera)
        {
            mCamera->onUpdate();

            const auto cameraData = mCamera->getCameraData();
            mCameraUniformBuffers[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
        }

        isFirstUpdate = false;
    }

    void onRender(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
    {
        if (mRenderPath)
        {
            mRenderPath->execute(commandList, frameData);
        }
    }

    template <class T>
    requires std::is_base_of_v<Object, T>
    void addObject(const SPtr<Geometry>& geometry, const int32_t tex, const Transform transform) noexcept
    {
        auto obj = makeUnique<T>();
        obj->pGeometry = geometry;
        obj->textureIndex = tex;
        obj->transform = transform;

        auto instanceData = obj->getInstanceData();
        instanceData.blasAddress = mGeometry->getGeometryBLAS(obj->pGeometry->getName())->getAddress();
        obj->instanceIndex = mInstancePool->acquire(instanceData);

        mObjects.push_back(std::move(obj));
    }

private:
    void initScene() noexcept
    {
        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        auto camera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 25.0f, 5.0f));

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
            const auto transform = Transform().translate({
                Random::get(-64.0f, 64.0f),
                Random::get(-64.0f, 64.0f),
                Random::get(-64.0f, 64.0f),
            });
            addObject<ExampleObject>(geoCube, 1, transform);
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
        }
    }

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
};
