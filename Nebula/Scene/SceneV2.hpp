#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "SceneGeometry.hpp"
#include "Core/Types.hpp"
#include "Geometry/Geometry.hpp"
#include "Geometry/Geometry.hpp"
#include "Math/Transform.hpp"
#include "VulkanRHI/Buffer.hpp"

struct Material
{
    glm::vec4   solidColor   = { 0.8f, 0.8f, 0.8f, 1.0f };
    int32_t     textureIndex = -1;

    int32_t     id;
    std::string name;
};

struct Object
{
    virtual      ~Object() = default;
    virtual void onUpdate(float dt) noexcept {}

    // Properties
    SPtr<Geometry>  pGeometry;
    SPtr<Material>  pMaterial;
    Transform       transform;

    // Raytracing Properties
    uint32_t        rt_hitGroup  = 0;
    uint32_t        rt_mask      = 0xff;

    // General
    int32_t         id;
    std::string     name;
};

class SceneV2
{
public:
    explicit SceneV2(const SPtr<RHI::VulkanRHI>& rhi)
    : mRHI(rhi)
    {
        mGeometry = makeUnique<SceneGeometry>(mRHI);

        mGeometry->addGeometry<Cube>(Cube::Params {});
        mGeometry->addGeometry<Sphere>(Sphere::Params {});

        mGeometry->onUpdate();

        mGeometry->addGeometry<Cube>(Cube::Params {});
        mGeometry->onUpdate();
    }

private:
    SPtr<RHI::VulkanRHI> mRHI;

    UPtr<SceneGeometry>  mGeometry;

};
