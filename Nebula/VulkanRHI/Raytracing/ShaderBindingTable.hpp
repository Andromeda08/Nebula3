#pragma once

#include <cstdint>
#include <string>
#include <vulkan/vulkan.hpp>

#include "Core/Macro.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    struct ShaderBindingTableCreateInfo
    {
        uint32_t        missCount = 0;
        uint32_t        hitCount  = 0;
        uint32_t        callableCount = 0;
        std::string     debugName;
        vk::Pipeline    pipeline;
        SPtr<Device>    device;
    };

    class ShaderBindingTable
    {
    public:
        nbl_DISABLE_COPY(ShaderBindingTable);
        nbl_CTOR_SHARED(ShaderBindingTable);

        const SPtr<Buffer>& getBuffer() const { return mBuffer; }

        const vk::StridedDeviceAddressRegionKHR* getRaygenRegion() const
        {
            return &mRaygenRegion;
        }

        const vk::StridedDeviceAddressRegionKHR* getMissRegion() const
        {
            return &mMissRegion;
        }

        const vk::StridedDeviceAddressRegionKHR* getHitRegion() const
        {
            return &mHitRegion;
        }

        const vk::StridedDeviceAddressRegionKHR* getCallRegion() const
        {
            return &mCallableRegion;
        }

    private:
        void getRaytracingProperties(vk::PhysicalDeviceRayTracingPipelinePropertiesKHR* pProps) const;

        /**
         * Round up sizes to next alignment
         * https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/alignment.hpp
         */
        template <class Integral>
        static constexpr Integral alignUp(Integral x, size_t a) noexcept
        {
            return Integral((x + (Integral(a) - 1)) & ~Integral(a - 1));
        }

        std::shared_ptr<Device> mDevice;
        std::shared_ptr<Buffer> mBuffer;
        vk::Pipeline            mPipeline;

        uint32_t mMissCount     = 0;
        uint32_t mHitCount      = 0;
        uint32_t mCallableCount = 0;

        vk::StridedDeviceAddressRegionKHR mRaygenRegion = {};
        vk::StridedDeviceAddressRegionKHR mMissRegion = {};
        vk::StridedDeviceAddressRegionKHR mHitRegion = {};
        vk::StridedDeviceAddressRegionKHR mCallableRegion = {};
    };
}
