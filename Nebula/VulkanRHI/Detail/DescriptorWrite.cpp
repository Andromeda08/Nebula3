#include "DescriptorWrite.hpp"

#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Raytracing/AccelerationStructure.hpp"

namespace RHI
{
    DescriptorWrite& DescriptorWrite::writeUniformBuffer(const uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept
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

    DescriptorWrite& DescriptorWrite::writeStorageBuffer(const uint32_t binding, const SPtr<Buffer>& pBuffer) noexcept
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

    DescriptorWrite& DescriptorWrite::writeStorageImage(const uint32_t binding, const vk::ImageLayout layout,
        const SPtr<Image>& pImage) noexcept
    {
        return writeStorageImages(binding, layout, {pImage});
    }

    DescriptorWrite& DescriptorWrite::writeStorageImages(const uint32_t binding, const vk::ImageLayout layout,
        const std::vector<SPtr<Image>>& images) noexcept
    {
        const auto key   = mWrites.size();
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

    DescriptorWrite& DescriptorWrite::writeCombinedImageSampler(
        const uint32_t                    binding,
        const uint32_t                    index,
        const vk::ImageLayout             layout,
        const SPtr<Image>&                pImage,
        const std::optional<vk::Sampler>& sampler) noexcept
    {
        const auto key   = mWrites.size();
        mImageInfos[key] = {
            vk::DescriptorImageInfo()
                .setImageLayout(layout)
                .setImageView(pImage->getImageView())
                .setSampler(sampler.value_or(pImage->getSampler()))
        };

        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDstArrayElement(index)
            .setPImageInfo(mImageInfos[key].data());
        mWrites.push_back(write);

        return *this;
    }

    DescriptorWrite& DescriptorWrite::writeCombinedImageSamplers(const uint32_t binding, const vk::ImageLayout layout,
        const std::vector<SPtr<Image>>& images) noexcept
    {
        const auto key   = mWrites.size();
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

    DescriptorWrite& DescriptorWrite::writeAccelerationStructure(const uint32_t binding,
        const SPtr<AccelerationStructure>& pTLAS) noexcept
    {
        exitOnAssert(pTLAS->getType() == AccelerationStructureType::TopLevel, "Only Top-Level AS can be used as in a descriptor set.");

        mASWrite = vk::WriteDescriptorSetAccelerationStructureKHR()
            .setAccelerationStructures(pTLAS->getHandle());

        const auto write = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDstArrayElement(0)
            .setPNext(&mASWrite);
        mWrites.push_back(write);

        return *this;
    }
}
