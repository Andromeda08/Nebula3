#include "Descriptor.hpp"

#include <print>

namespace RHI
{
    #pragma region "DescriptorWriteInfo"

    DescriptorWriteInfo& DescriptorWriteInfo::setSetIndex(const uint32_t index)
    {
        setIndex = index;
        return *this;
    }

    DescriptorWriteInfo& DescriptorWriteInfo::writeAccelerationStructure(
        const uint32_t                                        binding,
        const vk::WriteDescriptorSetAccelerationStructureKHR& accelerationStructure,
        const uint32_t                                        count)
    {
        accelerationStructureInfos.push_back(accelerationStructure);
        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(count)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDstArrayElement(0)
            .setPNext(&accelerationStructure);
        writes.push_back(write);
        return *this;
    }

    DescriptorWriteInfo& DescriptorWriteInfo::writeUniformBuffers(
        const uint32_t                  binding,
        const uint32_t                  bufferInfoCount,
        const vk::DescriptorBufferInfo* pBufferInfos)
    {
        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(bufferInfoCount)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDstArrayElement(0)
            .setPBufferInfo(pBufferInfos);
        writes.push_back(write);
        return *this;
    }

    DescriptorWriteInfo& DescriptorWriteInfo::writeStorageBuffers(
        const uint32_t                  binding,
        const uint32_t                  bufferInfoCount,
        const vk::DescriptorBufferInfo* pBufferInfos)
    {
        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(bufferInfoCount)
            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
            .setDstArrayElement(0)
            .setPBufferInfo(pBufferInfos);
        writes.push_back(write);
        return *this;
    }

    DescriptorWriteInfo& DescriptorWriteInfo::writeCombinedImageSamplers(
        const uint32_t                 binding,
        const uint32_t                 imageInfoCount,
        const vk::DescriptorImageInfo* pImageInfos,
        const uint32_t                 dstElement)
    {
        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(imageInfoCount)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDstArrayElement(dstElement)
            .setPImageInfo(pImageInfos);
        writes.push_back(write);
        return *this;
    }

    DescriptorWriteInfo& DescriptorWriteInfo::writeStorageImages(
        const uint32_t                 binding,
        const uint32_t                 imageInfoCount,
        const vk::DescriptorImageInfo* pImageInfos)
    {
        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(imageInfoCount)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setDstArrayElement(0)
            .setPImageInfo(pImageInfos);
        writes.push_back(write);
        return *this;
    }

    #pragma endregion

    Descriptor::Descriptor(const DescriptorCreateInfo& createInfo)
    : mBindings(createInfo.bindings)
    , mSetCount(createInfo.setCount)
    , mDebugName(createInfo.debugName)
    , mDevice(createInfo.device)
    {
        createPool();
        createLayout();
        createSets();

        if (createInfo.initialWrite.has_value())
        {
            const auto& initialWrite = createInfo.initialWrite.value();
            write(initialWrite);
        }
    }

    Descriptor::~Descriptor()
    {
        const vk::Device device = mDevice->getHandle();
        device.waitIdle();

        const vk::Result result = device.freeDescriptorSets(mDescriptorPool, mSetCount, mDescriptorSets.data());
        assert(result == vk::Result::eSuccess);

        device.destroyDescriptorPool(mDescriptorPool);
        device.destroyDescriptorSetLayout(mLayout);
    }

    void Descriptor::write(DescriptorWriteInfo writeInfo) const
    {
        assert(writeInfo.setIndex < mSetCount);
        if (writeInfo.writes.empty())
        {
            std::println("[RHI] Warning: Empty descriptor write for Descriptor {} set #{}", mDebugName, writeInfo.setIndex);
            return;
        }

        for (auto& write : writeInfo.writes)
        {
            write.setDstSet(mDescriptorSets[writeInfo.setIndex]);
        }

        mDevice->getHandle().updateDescriptorSets(writeInfo.writes, {});
    }

    vk::DescriptorSet Descriptor::getSet(const size_t i) const
    {
        assert(i < mSetCount);
        return mDescriptorSets[i];
    }

    vk::DescriptorSet Descriptor::operator[](const size_t i) const
    {
        return getSet(i);
    }

    void Descriptor::createPool()
    {
        std::vector<vk::DescriptorPoolSize> poolSizes;
        for (const auto& binding : mBindings)
        {
            poolSizes.push_back(vk::DescriptorPoolSize().setDescriptorCount(binding.descriptorCount * mSetCount).setType(binding.descriptorType));
        }

        const auto poolCreateInfo = vk::DescriptorPoolCreateInfo()
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
            .setMaxSets(mSetCount)
            .setPoolSizeCount(poolSizes.size())
            .setPPoolSizes(poolSizes.data());

        mDescriptorPool = mDevice->getHandle().createDescriptorPool(poolCreateInfo);

        mDevice->nameObject<vk::DescriptorPool>({
            .debugName = std::format("{} [DescriptorPool]", mDebugName),
            .handle    = mDescriptorPool,
        });
    }

    void Descriptor::createLayout()
    {
        const auto createInfo = vk::DescriptorSetLayoutCreateInfo()
            .setBindingCount(mBindings.size())
            .setPBindings(mBindings.data());

        mLayout = mDevice->getHandle().createDescriptorSetLayout(createInfo);

        mDevice->nameObject<vk::DescriptorSetLayout>({
            .debugName = std::format("{} [DescriptorLayout]", mDebugName),
            .handle    = mLayout,
        });
    }

    void Descriptor::createSets()
    {
        mDescriptorSets.resize(mSetCount);
        const std::vector layouts(mSetCount, mLayout);
        const auto allocateInfo = vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(mDescriptorPool)
            .setDescriptorSetCount(mSetCount)
            .setPSetLayouts(layouts.data());

        const vk::Result result = mDevice->getHandle().allocateDescriptorSets(&allocateInfo, mDescriptorSets.data());
        assert(result == vk::Result::eSuccess);

        for (const auto& [i, descriptorSet] : std::views::enumerate(mDescriptorSets))
        {
            mDevice->nameObject<vk::DescriptorSet>({
                .debugName = std::format("{} [DescriptorSet #{}]", mDebugName, i),
                .handle    = descriptorSet,
            });
        }
    }
}
