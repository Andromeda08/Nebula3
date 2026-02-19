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
    }

    SPtr<RHI::VulkanRHI>        mRHI;

    UPtr<SceneGeometry>         mGeometry;
    UPtr<TextureManager>        mTextureManager;

    std::vector<UPtr<Object>>   mObjects;
};
