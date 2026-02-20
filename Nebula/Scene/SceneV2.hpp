#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "InstancePool.hpp"
#include "SceneGeometry.hpp"
#include "Core/Random.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/Transform.hpp"

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
        mInstancePool = makeUnique<InstancePool>(mRHI);
        mTextureManager = TextureManager::create({ mRHI });

        initScene();
    }

    void onUpdate(const float dt, const RHI::CommandList* pCommandList) noexcept
    {
        for (auto& obj : mObjects)
        {
            obj->onUpdate(dt);
            if (obj->transform.isDirty())
            {
                mInstancePool->update(obj->instanceIndex, {
                .model         = obj->transform.getModel(),
                .solidColor    = obj->solidColor,
                .textureIndex  = obj->textureIndex,
                .geometryIndex = obj->geometryIndex,
            });
            }
        }
        mInstancePool->flush(pCommandList);
    }

    template <class T>
    requires std::is_base_of_v<Object, T>
    void addObject(const SPtr<Geometry>& geometry, const int32_t tex, const Transform transform) noexcept
    {
        auto obj = makeUnique<T>();
        obj->pGeometry = geometry;
        obj->textureIndex = tex;
        obj->transform = transform;

        obj->instanceIndex = mInstancePool->acquire(obj->getInstanceData());

        mObjects.push_back(std::move(obj));
    }

private:
    void initScene() noexcept
    {
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

        buildTLAS();
    }

    void buildTLAS() noexcept
    {
        // Gather instance data
        // ===================================
        #pragma region
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        for (const auto& obj : mObjects)
        {
            const auto blas = mGeometry->getGeometryBLAS(obj->pGeometry->getName());
            const auto instance = vk::AccelerationStructureInstanceKHR()
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setMask(obj->rt_mask)
                .setAccelerationStructureReference(blas->getAddress())
                .setInstanceShaderBindingTableRecordOffset(0)
                .setTransform(obj->transform.getModel3x4());
            instances.push_back(instance);
        }

        const auto instanceSize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        mTopLevelInstances = mRHI->createBuffer({
            instanceSize, RHI::BufferType::Storage
        });
        mRHI->immediate_uploadToBuffer(mTopLevelInstances.get(), instances.data(), instanceSize, 0);
        #pragma endregion

        // Prepare Top Level Build
        // ===================================
        #pragma region
        const auto geometryInstancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
            .setArrayOfPointers(false)
            .setData(mTopLevelInstances->getAddress());

        const auto tlasGeometryData = vk::AccelerationStructureGeometryDataKHR()
            .setInstances(geometryInstancesData);

        const auto tlasGeometry = vk::AccelerationStructureGeometryKHR()
            .setFlags({})
            .setGeometry(tlasGeometryData)
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setPNext(nullptr);

        auto tlasBuildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometryCount(1)
            .setPGeometries(&tlasGeometry)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel);

        const auto tlasBuildSizesInfo = mRHI->getDevice()->getHandle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, tlasBuildGeometryInfo, instances.size());
        #pragma endregion

        mTopLevelData = mRHI->createBuffer({ tlasBuildSizesInfo.accelerationStructureSize, RHI::BufferType::AccelerationStructure });
        mTopLevelAS = RHI::AccelerationStructure::create({
            .backingBuffer = mTopLevelData,
            .offset = 0,
            .size = tlasBuildSizesInfo.accelerationStructureSize,
            .type = RHI::AccelerationStructureType::TopLevel,
            .label = "Scene-TopLevelAS"
        }, mRHI->getDevice());

        const auto buildScratch = mRHI->createBuffer({ tlasBuildSizesInfo.buildScratchSize, RHI::BufferType::Storage });
        tlasBuildGeometryInfo
            .setDstAccelerationStructure(mTopLevelAS->getHandle())
            .setScratchData(buildScratch->getAddress());

        const auto tlasBuildRangeInfo = vk::AccelerationStructureBuildRangeInfoKHR().setPrimitiveCount(instances.size());
        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> tlasBuildRanges = {&tlasBuildRangeInfo};

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void
        {
           pCommandList->getHandle().buildAccelerationStructuresKHR(tlasBuildRanges.size(), &tlasBuildGeometryInfo, tlasBuildRanges.data());
        });
    }

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<SceneGeometry>                 mGeometry;
    UPtr<InstancePool>                  mInstancePool;
    UPtr<TextureManager>                mTextureManager;

    std::vector<UPtr<Object>>           mObjects;

    SPtr<RHI::AccelerationStructure>    mTopLevelAS;
    SPtr<RHI::Buffer>                   mTopLevelInstances;
    SPtr<RHI::Buffer>                   mTopLevelData;
};
