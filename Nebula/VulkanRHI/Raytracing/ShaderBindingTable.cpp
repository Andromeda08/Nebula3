#include "ShaderBindingTable.hpp"

namespace RHI
{
    ShaderBindingTable::ShaderBindingTable(const ShaderBindingTableCreateInfo& createInfo)
    : mDevice(createInfo.device)
    , mPipeline(createInfo.pipeline)
    , mMissCount(createInfo.missCount)
    , mHitCount(createInfo.hitCount)
    , mCallableCount(createInfo.callableCount)
    {
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rt_props;
        getRaytracingProperties(&rt_props);

        uint32_t shaderGroupHandleSize = rt_props.shaderGroupHandleSize;
        uint32_t shaderGroupBaseAlignment = rt_props.shaderGroupBaseAlignment;
        uint32_t handleCount = 1 + mMissCount + mHitCount + mCallableCount;

        uint32_t alignedHandleSize = alignUp(shaderGroupHandleSize, rt_props.shaderGroupHandleAlignment);

        uint32_t raygenStride = alignUp(alignedHandleSize, shaderGroupBaseAlignment);
        mRaygenRegion = vk::StridedDeviceAddressRegionKHR()
            .setSize(raygenStride)
            .setStride(raygenStride);

        mMissRegion = vk::StridedDeviceAddressRegionKHR()
            .setSize(alignUp(mMissCount * alignedHandleSize, shaderGroupBaseAlignment))
            .setStride(alignedHandleSize);

        mHitRegion = vk::StridedDeviceAddressRegionKHR()
            .setSize(alignUp(mHitCount * alignedHandleSize, shaderGroupBaseAlignment))
            .setStride(alignedHandleSize);

        mCallableRegion = vk::StridedDeviceAddressRegionKHR()
            .setSize(alignUp(mCallableCount * alignedHandleSize, shaderGroupBaseAlignment))
            .setStride(alignedHandleSize);

        uint32_t dataSize = handleCount * shaderGroupHandleSize;
        std::vector<uint8_t> handles(dataSize);

        const vk::Result result = mDevice->getHandle().getRayTracingShaderGroupHandlesKHR(mPipeline, 0, handleCount, dataSize, handles.data());
        assert(result == vk::Result::eSuccess);
        
        vk::DeviceSize sbtSize = mRaygenRegion.size + mMissRegion.size + mHitRegion.size + mCallableRegion.size;

        BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.size      = sbtSize;
        bufferCreateInfo.type      = BufferType::ShaderBindingTable;
        bufferCreateInfo.label     = createInfo.debugName;
        bufferCreateInfo.device    = mDevice;

        mBuffer = Buffer::create(bufferCreateInfo);

        auto sbt_address = mBuffer->getAddress();
        mRaygenRegion.setDeviceAddress(sbt_address);
        mMissRegion.setDeviceAddress(sbt_address + mRaygenRegion.size);
        mHitRegion.setDeviceAddress(sbt_address + mRaygenRegion.size + mMissRegion.size);
        mCallableRegion.setDeviceAddress(sbt_address + mRaygenRegion.size + mMissRegion.size + mHitRegion.size);

        auto get_handle = [&](uint32_t i) { return handles.data() + i * shaderGroupHandleSize; };

        void* mappedMemory = mBuffer->map();
        
        uint8_t* pSBT = reinterpret_cast<uint8_t*>(mappedMemory);
        uint8_t* pData = nullptr;
        uint32_t handleIdx = 0;

        pData = pSBT;
        std::memcpy(pData, get_handle(handleIdx++), shaderGroupHandleSize);

        pData = reinterpret_cast<uint8_t*>(mappedMemory) + mRaygenRegion.size;
        for (uint32_t i = 0; i < mMissCount; i++)
        {
            std::memcpy(pData, get_handle(handleIdx++), shaderGroupHandleSize);
            pData += mMissRegion.stride;
        }

        pData = reinterpret_cast<uint8_t*>(mappedMemory) + mRaygenRegion.size + mMissRegion.size;
        for (uint32_t i = 0; i < mHitCount; i++)
        {
            std::memcpy(pData, get_handle(handleIdx++), shaderGroupHandleSize);
            pData += mHitRegion.stride;
        }


        pData = reinterpret_cast<uint8_t*>(mappedMemory) + mRaygenRegion.size + mMissRegion.size + mHitRegion.size;
        for (uint32_t i = 0; i < mCallableCount; i++)
        {
            std::memcpy(pData, get_handle(handleIdx++), shaderGroupHandleSize);
            pData += mCallableRegion.stride;
        }
    }

    void ShaderBindingTable::getRaytracingProperties(vk::PhysicalDeviceRayTracingPipelinePropertiesKHR* pProps) const
    {
        vk::PhysicalDeviceProperties2 props2;
        props2.pNext = pProps;
        mDevice->getPhysicalDevice().getProperties2(&props2);
    }
}
