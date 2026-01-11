#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/DescriptorWrite.hpp"

namespace RHI
{
    struct DescriptorWriteInfo
    {
        uint32_t                              setIndex = 0;
        std::vector<vk::WriteDescriptorSet>   writes;
        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo>  imageInfos;
        std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelerationStructureInfos;

        DescriptorWriteInfo& setSetIndex(uint32_t index);

        DescriptorWriteInfo& writeAccelerationStructure(uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR& accelerationStructure, uint32_t count);

        DescriptorWriteInfo& writeUniformBuffers(uint32_t binding, uint32_t bufferInfoCount, const vk::DescriptorBufferInfo* pBufferInfos);

        DescriptorWriteInfo& writeStorageBuffers(uint32_t binding, uint32_t bufferInfoCount, const vk::DescriptorBufferInfo* pBufferInfos);

        DescriptorWriteInfo& writeCombinedImageSamplers(uint32_t binding, uint32_t imageInfoCount, const vk::DescriptorImageInfo* pImageInfos, uint32_t dstElement = 0);

        DescriptorWriteInfo& writeStorageImages(uint32_t binding, uint32_t imageInfoCount, const vk::DescriptorImageInfo* pImageInfos);
    };

    struct RHIDescriptorCreateInfo
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings     = {};
        uint32_t                                    setCount     = 1;
        std::string                                 debugName    = "Unknown Descriptor";
        std::optional<DescriptorWriteInfo>          initialWrite = std::nullopt;
    };

    struct DescriptorCreateInfo : public RHIDescriptorCreateInfo
    {
        SPtr<Device> device = nullptr;
    };

    class Descriptor
    {
    public:
        nbl_DISABLE_COPY(Descriptor);
        nbl_CTOR_SHARED(Descriptor);

        ~Descriptor();

        [[deprecated("Use the new write() method and DescriptorWrite class")]] void write_old(DescriptorWriteInfo writeInfo) const;

        void write(uint32_t setIndex, const DescriptorWrite& descriptorWrite) const noexcept;

        void writeAll(const DescriptorWrite& descriptorWrite) const noexcept;

        vk::DescriptorSet getSet(size_t i = 0) const;

        vk::DescriptorSet operator[](size_t i) const;

        vk::DescriptorSetLayout getLayout() const noexcept
        {
            return mLayout;
        }

        uint32_t getSetCount() const noexcept
        {
            return mSetCount;
        }

    private:
        void createPool();

        void createLayout();

        void createSets();

        std::vector<vk::DescriptorSet>              mDescriptorSets;
        std::vector<vk::DescriptorSetLayoutBinding> mBindings;
        vk::DescriptorSetLayout                     mLayout;
        vk::DescriptorPool                          mDescriptorPool;

        const uint32_t                              mSetCount;
        const std::string                           mDebugName;

        SPtr<Device>                                mDevice;
    };
}
