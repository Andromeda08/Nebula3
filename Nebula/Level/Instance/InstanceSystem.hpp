#pragma once

#include <glm/glm.hpp>
#include "../SlotPool.hpp"
#include "Math/BoundingBox.hpp"

namespace nbl
{
    struct GPUInstanceData
    {
        glm::mat4   model;
        glm::vec4   aabbMin;
        glm::vec4   aabbMax;
        uint64_t    blas;
        int32_t     geometryIndex;
        int32_t     materialIndex;
        int32_t     objectId;
        int32_t     _pad0 = 0;
        int32_t     _pad1 = 0;
        int32_t     _pad2 = 0;
    };

    struct InstanceData
    {
        glm::mat4   model;

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

        [[nodiscard]] GPUInstanceData toGPU() const
        {
            return {
                .model          = model,
                .aabbMin        = glm::vec4(boundingBox.getMin(), 1.0f),
                .aabbMax        = glm::vec4(boundingBox.getMax(), 1.0f),
                .blas           = blas,
                .geometryIndex  = geometryIndex,
                .materialIndex  = materialIndex,
                .objectId       = objectId,
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