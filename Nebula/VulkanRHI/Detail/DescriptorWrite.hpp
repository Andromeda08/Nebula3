#pragma once

#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class DescriptorWrite
    {
    public:
        DescriptorWrite() = default;

        DescriptorWrite& addSetIndex(const uint32_t index) noexcept
        {
            mWriteSetIndices.insert(index);
            return *this;
        }

        template <std::ranges::input_range Range>
        DescriptorWrite& addSetIndices(Range&& indices) noexcept
        {
            mWriteSetIndices.insert_range(indices);
            return *this;
        }

        DescriptorWrite& writeUniformBuffer(const uint32_t binding, const vk::DescriptorBufferInfo& bufferInfo) noexcept
        {
            mBufferInfos[binding].push_back(bufferInfo);
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mBufferInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstArrayElement(0)
                .setPBufferInfo(mBufferInfos[binding].data());
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeStorageBuffer(const uint32_t binding, const vk::DescriptorBufferInfo& bufferInfo) noexcept
        {
            mBufferInfos[binding].push_back(bufferInfo);
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mBufferInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDstArrayElement(0)
                .setPBufferInfo(mBufferInfos[binding].data());
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeCombinedImageSamplers(const uint32_t binding, const std::vector<vk::DescriptorImageInfo>& imageInfos) noexcept
        {
            mImageInfos[binding].append_range(imageInfos);
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[binding].data());
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeCombinedImageSampler(const uint32_t binding, const uint32_t dstElement, const vk::DescriptorImageInfo& imageInfo) noexcept
        {
            mImageInfos[binding].push_back(imageInfo);
            const auto* pImageInfo = &mImageInfos[binding].back();
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstArrayElement(dstElement)
                .setPImageInfo(pImageInfo);
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeStorageImages(const uint32_t binding, const std::vector<vk::DescriptorImageInfo>& imageInfos) noexcept
        {
            mImageInfos[binding].append_range(imageInfos);
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[binding].data());
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeStorageImage(const uint32_t binding, const uint32_t dstElement, const vk::DescriptorImageInfo& imageInfo) noexcept
        {
            mImageInfos[binding].push_back(imageInfo);
            const auto* pImageInfo = &mImageInfos[binding].back();
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[binding].size())
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDstArrayElement(dstElement)
                .setPImageInfo(pImageInfo);
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeAccelerationStructure(const uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR& asInfo) noexcept
        {
            mASInfo = asInfo;
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                .setDstArrayElement(0)
                .setPNext(&mASInfo.value());
            mWrites.push_back(write);
            return *this;
        }

    private:
        friend class Descriptor;

        std::optional<vk::WriteDescriptorSetAccelerationStructureKHR>       mASInfo = std::nullopt;
        std::unordered_map<uint32_t, std::vector<vk::DescriptorBufferInfo>> mBufferInfos;
        std::unordered_map<uint32_t, std::vector<vk::DescriptorImageInfo>>  mImageInfos;
        std::vector<vk::WriteDescriptorSet>                                 mWrites;
        std::set<uint32_t>                                                  mWriteSetIndices;
    };
}
