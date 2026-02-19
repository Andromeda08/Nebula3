#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "SceneGeometry.hpp"
#include "Core/Random.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/Transform.hpp"

struct GPUObjectInstanceDataV2
{
    glm::mat4 model;
    int32_t   textureIndex;
    int32_t   instanceIndex;
    int32_t   _p0, _p1;
};

struct Object
{
    virtual      ~Object() = default;
    virtual void onUpdate(float dt) noexcept {}

    // Properties
    SPtr<Geometry> pGeometry    = nullptr;
    int32_t        textureIndex = -1;
    glm::vec4      solidColor   = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    Transform      transform    = {};

    // Raytracing Properties
    uint32_t        rt_hitGroup  = 0;
    uint32_t        rt_mask      = 0xff;

    // General
    int32_t         id;
    std::string     name;
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
        mTextureManager = TextureManager::create({ mRHI });

        initScene();
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

        auto cube = makeUnique<ExampleObject>();
        cube->pGeometry = geoCube;
        cube->textureIndex = 1;
        cube->transform.translate({ 5.0f, 0.0f, 0.0f });
        mObjects.push_back(std::move(cube));

        auto sphere = makeUnique<ExampleObject>();
        sphere->pGeometry = geoSphere;
        sphere->textureIndex = 2;
        sphere->transform.translate({ 0.0f, 0.0f, 0.0f });
        mObjects.push_back(std::move(sphere));

        auto cylinder = makeUnique<ExampleObject>();
        cylinder->pGeometry = geoCylinder;
        cylinder->textureIndex = 3;
        cylinder->transform.translate({ -5.0f, 0.0f, 0.0f });
        mObjects.push_back(std::move(cylinder));

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
            const auto m = obj->transform.getModel();
            const auto instance = vk::AccelerationStructureInstanceKHR()
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setMask(obj->rt_mask)
                .setAccelerationStructureReference(blas->getAddress())
                .setInstanceShaderBindingTableRecordOffset(0)
                .setTransform(vk::TransformMatrixKHR({
                    std::array { m[0].x, m[1].x, m[2].x, m[3].x },
                    std::array { m[0].y, m[1].y, m[2].y, m[3].y },
                    std::array { m[0].z, m[1].z, m[2].z, m[3].z }
                }));
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
    UPtr<TextureManager>                mTextureManager;

    std::vector<UPtr<Object>>           mObjects;

    SPtr<RHI::AccelerationStructure>    mTopLevelAS;
    SPtr<RHI::Buffer>                   mTopLevelInstances;
    SPtr<RHI::Buffer>                   mTopLevelData;
};
