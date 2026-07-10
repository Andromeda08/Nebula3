#pragma once

#include <string>

#include "Level/SlotPool.hpp"
#include "Level/Transform.hpp"
#include "Math/BoundingBox.hpp"

namespace nbl
{
    struct Object
    {
        virtual      ~Object() = default;

        virtual void onUpdate(float dt) noexcept {}

        int32_t     id              = -1;
        std::string name            = "Unknown";
        Object*     pParent         = nullptr;

        int32_t     geometryIndex   = -1;
        Transform   transform       = {};
        Handle      hMaterial       = {};
        uint64_t    blasAddress     = {};

        bool        isInstanceDirty = true;
        bool        isFirstUpdate   = true;
        Handle      hInstance       = {};

        int32_t     emitterIndex    = -1;

        [[nodiscard]] glm::mat4 getModel() const
        {
            const glm::mat4 local = transform.getModel();
            if (pParent)
            {
                return pParent->getModel() * local;
            }
            return local;
        }
    };
}
