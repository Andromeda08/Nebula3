#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "InstancePool.hpp"
#include "SceneGeometry.hpp"
#include "TLASManager.hpp"
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

        mTLASManager = TLASManager::create({ mRHI, mInstancePool.get() });

        initScene();
    }

    void onUpdate(const float dt, const RHI::CommandList* pCommandList) noexcept
    {
        for (const auto& obj : mObjects)
        {
            obj->onUpdate(dt);
            if (obj->transform.isDirty())
            {
                auto instanceData = obj->getInstanceData();
                instanceData.blasAddress = mGeometry->getGeometryBLAS(obj->pGeometry->getName())->getAddress();

                mInstancePool->update(obj->instanceIndex, instanceData);
            }
        }
        mInstancePool->flush(pCommandList);
        mTLASManager->onUpdate(pCommandList);
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
    }

    SPtr<RHI::VulkanRHI>                mRHI;

    UPtr<SceneGeometry>                 mGeometry;
    UPtr<InstancePool>                  mInstancePool;
    UPtr<TextureManager>                mTextureManager;
    UPtr<TLASManager>                   mTLASManager;

    std::vector<UPtr<Object>>           mObjects;
};
