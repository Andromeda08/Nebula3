#pragma once

#include <SDL3/SDL_events.h>

#include "Geometry/GeometrySystem.hpp"
#include "Instance/InstancePool.hpp"
#include "Material/MaterialSystem.hpp"
#include "Math/BoundingBox.hpp"
#include "Math/Transform.hpp"

namespace nbl
{
    struct ObjectParams
    {
        std::string     name;

        Geometry*       pGeometry;
        GeometryMetadata geometryInfo;
        GeometryIndex   geometryIndex;
        MaterialIndex   materialIndex;
        InstanceIndex   instanceIndex;

        Transform       transform;

        uint32_t        rtHitGroup  = 0;
        uint32_t        rtMask      = 0xff;
    };

    class Object
    {
    public:
        explicit Object(const ObjectParams& params)
        : mName(params.name)
        , mGeometry(params.pGeometry)
        , mGeometryInfo(params.geometryInfo)
        , mGeometryIndex(params.geometryIndex)
        , mMaterialIndex(params.materialIndex)
        , mInstanceIndex(params.instanceIndex)
        , mTransform(params.transform)
        , mRtHitGroup(params.rtHitGroup)
        , mRtMask(params.rtMask)
        {
            mTransformedBounds = mGeometry->getBoundingBox().getTransformed(mTransform.getModel());
        }

        virtual ~Object() = default;

        virtual void onEvent(const SDL_Event& event) {}

        virtual void onUpdate(float dt) {}

        std::string         mName;

        Geometry*           mGeometry;
        GeometryMetadata    mGeometryInfo;
        GeometryIndex       mGeometryIndex = -1;
        MaterialIndex       mMaterialIndex = -1;
        InstanceIndex       mInstanceIndex = -1;

        Transform           mTransform     = {};

        uint32_t            mRtHitGroup    = 0;
        uint32_t            mRtMask        = 0xff;

        BoundingBox         mTransformedBounds;
    };
}
