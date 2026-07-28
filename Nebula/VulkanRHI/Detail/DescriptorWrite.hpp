#pragma once

#include "Core/Types.hpp"
#include <vulkan/vulkan.hpp>

namespace RHI
{
    class Buffer;
    class Image;
    class AccelerationStructure;

    class DescriptorWrite
    {
    public:
        // =============================
        // Buffer Descriptors
        // =============================

        DescriptorWrite& writeUniformBuffer(uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept;

        DescriptorWrite& writeStorageBuffer(uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept;

        // =============================
        // Storage Image
        // =============================

        DescriptorWrite& writeStorageImage(uint32_t binding, vk::ImageLayout layout, const SPtr<Image>& pImage) noexcept;

        DescriptorWrite& writeStorageImages(uint32_t binding, vk::ImageLayout layout, const std::vector<SPtr<Image>>& images) noexcept;

        template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, vk::DescriptorImageInfo>
        DescriptorWrite& writeStorageImageInfos(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key   = mWrites.size();
            mImageInfos[key] = imageInfos | std::ranges::to<std::vector>();

            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[key].size())
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        // =============================
        // Combined Image Sampler
        // =============================

        DescriptorWrite& writeCombinedImageSampler(uint32_t binding, uint32_t index, vk::ImageLayout layout, const SPtr<Image>& pImage, const std::optional<vk::Sampler>& sampler = std::nullopt) noexcept;

        DescriptorWrite& writeCombinedImageSamplers(uint32_t binding, vk::ImageLayout layout, const std::vector<SPtr<Image>>& images) noexcept;

        template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, vk::DescriptorImageInfo>
        DescriptorWrite& writeCombinedImageSamplerInfos(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key   = mWrites.size();
            mImageInfos[key] = imageInfos | std::ranges::to<std::vector>();

            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[key].size())
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, vk::DescriptorImageInfo>
        DescriptorWrite& writeSampledImages(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key   = mWrites.size();
            mImageInfos[key] = imageInfos | std::ranges::to<std::vector>();

            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[key].size())
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, vk::DescriptorImageInfo>
        DescriptorWrite& writeSamplers(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key   = mWrites.size();
            mImageInfos[key] = imageInfos | std::ranges::to<std::vector>();

            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(mImageInfos[key].size())
                .setDescriptorType(vk::DescriptorType::eSampler)
                .setDstArrayElement(0)
                .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        // =============================
        // Acceleration Structures
        // =============================

        DescriptorWrite& writeAccelerationStructure(uint32_t binding, const SPtr<AccelerationStructure>& pTLAS) noexcept;

    private:
        friend class Descriptor;

        vk::WriteDescriptorSetAccelerationStructureKHR              mASWrite;
        std::map<std::size_t, std::vector<vk::DescriptorImageInfo>> mImageInfos = {};
        std::vector<vk::WriteDescriptorSet> mWrites = {};
    };
}
