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

        DescriptorWrite& writeUniformBuffer(const uint32_t binding, Buffer* pBuffer) noexcept
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

        DescriptorWrite& writeStorageBuffer(const uint32_t binding, Buffer* pBuffer) noexcept
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
        // Image Descriptors
        // =============================

        DescriptorWrite& writeStorageImages(const uint32_t binding, const vk::ImageLayout layout, const std::vector<Image*>& images) noexcept
        {
            const auto imageInfos = images
                | std::views::transform([&](const auto& image){
                    return vk::DescriptorImageInfo()
                        .setImageLayout(layout)
                        .setImageView(image->getImageView())
                        .setSampler(nullptr);
                })
                | std::ranges::to<std::vector>();

            mImageInfos.push_back(imageInfos);

            const auto write = vk::WriteDescriptorSet()
                    .setDstBinding(binding)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eStorageImage)
                    .setDstArrayElement(0)
                    .setPImageInfo(mImageInfos.back().data());
            mWrites.push_back(write);

            return *this;
        }

        DescriptorWrite& writeCombinedImageSamplers(const uint32_t binding, const vk::ImageLayout layout, const std::vector<Image*>& images) noexcept
        {
            const auto imageInfos = images
                | std::views::transform([&](const auto& image){
                    return vk::DescriptorImageInfo()
                        .setImageLayout(layout)
                        .setImageView(image->getImageView())
                        .setSampler(image->getSampler());
                })
                | std::ranges::to<std::vector>();

            mImageInfos.push_back(imageInfos);

            const auto write = vk::WriteDescriptorSet()
                    .setDstBinding(binding)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDstArrayElement(0)
                    .setPImageInfo(mImageInfos.back().data());
            mWrites.push_back(write);

            return *this;
        }

        // =============================
        // Acceleration Structures
        // =============================

        // TODO: DescriptorWrite& writeAccelerationStructure(const uint32_t binding, TopLevelAS* pTLAS) noexcept;

    private:
        friend class Descriptor;

        std::vector<std::vector<vk::DescriptorImageInfo>> mImageInfos = {};
        std::vector<vk::WriteDescriptorSet> mWrites = {};
    };
}
