#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Core/Macro.hpp"
#include "Detail/BufferTraits.hpp"
#include "Detail/Resource.hpp"

namespace RHI
{
    struct RHIBufferCreateInfo
    {
        uint64_t    size = 0;
        BufferType  type = BufferType::Storage;
        std::string label;
    };

    struct BufferCreateInfo : RHIBufferCreateInfo
    {
        SPtr<Device> device = nullptr;
    };

    class Buffer : public Resource
    {
    public:
        nbl_DISABLE_COPY(Buffer);
        nbl_CTOR_SHARED(Buffer);

        ~Buffer() override;

        /**
         * Get the Descriptor Info for this Buffer (offset=0, range=size)
         * @param range Optionally specifiable range
         * @return Pointer to DescriptorBufferInfo
         */
        [[nodiscard]] vk::DescriptorBufferInfo* getDescriptorInfo(const std::optional<vk::DeviceSize>& range = std::nullopt) noexcept;

        void* map() const;

        void unmap() const;

        void setData(const void* pData, uint64_t size, uint64_t offset = 0) const;

        void readBack(void* pData, uint64_t size, uint64_t offset = 0) const;

        [[nodiscard]] vk::BufferMemoryBarrier2 getBarrier(BufferUsage srcUsage, BufferUsage dstUsage) const;

        const vk::Buffer& getHandle()    const { return mBuffer; }
        uint64_t          getSize()      const { return mProperties.size; }
        BufferType        getType()      const { return mProperties.type; }
        uint64_t          getAddress()   const { return mDeviceAddress; }

    private:
        const BufferProperties  mProperties;
        vk::Buffer              mBuffer;
        vk::DeviceAddress       mDeviceAddress;

        std::optional<vk::DescriptorBufferInfo> mDescriptorBufferInfo = std::nullopt;
    };
}
