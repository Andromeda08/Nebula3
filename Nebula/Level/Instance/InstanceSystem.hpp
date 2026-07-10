#pragma once

#include <glm/glm.hpp>
#include "../SlotPool.hpp"
#include "Math/BoundingBox.hpp"

namespace nbl
{
    struct GPUInstanceData
    {
        glm::mat4   model;
        glm::mat4   modelInverse;
        glm::mat4   modelPrevious;
        glm::vec4   aabbMin;
        glm::vec4   aabbMax;
        uint64_t    blas;
        int32_t     geometryIndex;
        int32_t     materialIndex;
        int32_t     objectId;
        int32_t     emitterIndex;
    };

    struct InstanceData
    {
        glm::mat4   model;
        glm::mat4   previousModel;

        /**
         * Expects the already transformed BoundingBox.
         * Objects shouldn't store bboxes, since the transformed bbox
         * has to be computed from the original geometries anyway.
         */
        BoundingBox boundingBox;

        uint64_t    blas;
        int32_t     geometryIndex;
        int32_t     materialIndex;
        int32_t     objectId;
        int32_t     emitterIndex = -1;

        [[nodiscard]] GPUInstanceData toGPU() const
        {
            return {
                .model          = model,
                .modelInverse   = glm::inverse(model),
                .modelPrevious  = previousModel,
                .aabbMin        = glm::vec4(boundingBox.getMin(), 1.0f),
                .aabbMax        = glm::vec4(boundingBox.getMax(), 1.0f),
                .blas           = blas,
                .geometryIndex  = geometryIndex,
                .materialIndex  = materialIndex,
                .objectId       = objectId,
                .emitterIndex   = emitterIndex,
            };
        }
    };

    class InstanceSystem : public Pool<InstanceData, GPUInstanceData>
    {
    public:
        explicit InstanceSystem(const SPtr<RHI::VulkanRHI>& rhi, const uint32_t capacity = 65536)
        : Pool(rhi, "InstanceSystem", capacity)
        {
        }

        void onUpdate(const RHI::CommandList* commandList)
        {
            flush(commandList);
        }
    };
}