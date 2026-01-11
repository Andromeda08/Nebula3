#pragma once

#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"

namespace RHI
{
    class DescriptorWrite
    {
    public:
        // =============================
        // Buffer Descriptors
        // =============================

        DescriptorWrite& writeUniformBuffer(const uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept
        {
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstArrayElement(0)
                .setPBufferInfo(pBuffer->getDescriptorInfo());
            mWrites.push_back(write);
            return *this;
        }

        DescriptorWrite& writeStorageBuffer(const uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept
        {
            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDstArrayElement(0)
                .setPBufferInfo(pBuffer->getDescriptorInfo());
            mWrites.push_back(write);
            return *this;
        }

        // =============================
        // Storage Image
        // =============================

        DescriptorWrite& writeStorageImage(const uint32_t binding, const vk::ImageLayout layout, const SPtr<Image>& pImage) noexcept
        {
            return writeStorageImages(binding, layout, {pImage});
        }

        DescriptorWrite& writeStorageImages(const uint32_t binding, const vk::ImageLayout layout, const std::vector<SPtr<Image>>& images) noexcept
        {
            const auto key = mWrites.size();
            mImageInfos[key] = images
                | std::views::transform([&](const auto& image){
                    return vk::DescriptorImageInfo()
                        .setImageLayout(layout)
                        .setImageView(image->getImageView())
                        .setSampler(nullptr);
                })
                | std::ranges::to<std::vector>();

            const auto write = vk::WriteDescriptorSet()
                    .setDstBinding(binding)
                    .setDescriptorCount(mImageInfos[key].size())
                    .setDescriptorType(vk::DescriptorType::eStorageImage)
                    .setDstArrayElement(0)
                    .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, vk::DescriptorImageInfo>
        DescriptorWrite& writeStorageImageInfos(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key = mWrites.size();
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

        DescriptorWrite& writeCombinedImageSampler(const uint32_t binding, const uint32_t index, const vk::ImageLayout layout, const SPtr<Image>& pImage) noexcept
        {
            const auto key = mWrites.size();
            mImageInfos[key] = {vk::DescriptorImageInfo()
                .setImageLayout(layout)
                .setImageView(pImage->getImageView())
                .setSampler(pImage->getSampler())};

            const auto write = vk::WriteDescriptorSet()
                .setDstBinding(binding)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstArrayElement(index)
                .setPImageInfo(mImageInfos[key].data());
            mWrites.push_back(write);

            return *this;
        }

        DescriptorWrite& writeCombinedImageSamplers(const uint32_t binding, const vk::ImageLayout layout, const std::vector<SPtr<Image>>& images) noexcept
        {
            const auto key = mWrites.size();
            mImageInfos[key] = images
                | std::views::transform([&](const auto& image){
                    return vk::DescriptorImageInfo()
                        .setImageLayout(layout)
                        .setImageView(image->getImageView())
                        .setSampler(image->getSampler());
                })
                | std::ranges::to<std::vector>();

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
        DescriptorWrite& writeCombinedImageSamplerInfos(const uint32_t binding, Range&& imageInfos) noexcept
        {
            const auto key = mWrites.size();
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

        // =============================
        // Acceleration Structures
        // =============================

        // TODO: DescriptorWrite& writeAccelerationStructure(const uint32_t binding, TopLevelAS* pTLAS) noexcept;

    private:
        friend class Descriptor;

        std::map<std::size_t, std::vector<vk::DescriptorImageInfo>> mImageInfos = {};
        std::vector<vk::WriteDescriptorSet> mWrites = {};
    };
}
