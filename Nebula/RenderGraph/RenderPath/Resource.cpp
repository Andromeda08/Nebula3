#include "Resource.hpp"

#include <format>
#include <stdexcept>

namespace rg
{
    Resource::Resource(std::string name, const ResourceType resourceType)
    : mName(std::move(name))
    , mResourceType(resourceType)
    {
    }

    const std::string& Resource::getName() const noexcept
    {
        return mName;
    }

    ResourceType Resource::getResourceType() const noexcept
    {
        return mResourceType;
    }

    SceneDataResource::SceneDataResource(std::string name)
    : Resource(std::move(name), ResourceType::SceneData)
    {
    }

    ImageResource::ImageResource(const SPtr<RHI::Image>& image, std::string name)
    : Resource(std::move(name), ResourceType::Image)
    {
        mImages.push_back(image);
    }

    ImageResource::ImageResource(
        const std::vector<SPtr<RHI::Image>>& images,
        const RHI::Allocation&               allocation,
        std::string                          name)
    : Resource(std::move(name), ResourceType::Image)
    , mImages(images)
    , mAliasedMemory(allocation)
    , mIsAliased(true)
    {
        for (auto& image : images)
        {
            image->useAllocation(mAliasedMemory->getAllocation(), mAliasedMemory->getAllocationInfo());
        }
    }

    SPtr<RHI::Image> ImageResource::getImage(const uint32_t i) const noexcept
    {
        assert(i < mImages.size());
        return mImages[i];
    }

    vk::RenderingAttachmentInfo ImageResource::getBasicAttachmentInfo(const uint32_t i) const noexcept
    {
        return vk::RenderingAttachmentInfo()
            .setClearValue(vk::ClearValue().setColor({ 0.0f, 0.0f, 0.0f, 1.0f }))
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImageView(mImages[i]->getImageView())
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore);
    }

    RHI::Attachment ImageResource::getBasicAttachment(const uint32_t i) const noexcept
    {
        return {
            .image          = mImages[i]->getImage(),
            .attachmentInfo = getBasicAttachmentInfo(i),
        };
    }

    ResourceFactory::ResourceFactory(const SPtr<RHI::VulkanRHI>& rhi): mRHI(rhi)
    {
    }

    SPtr<Resource> ResourceFactory::create(const OptimizerResource& resourceTemplate) const
    {
        switch (resourceTemplate.resourceType)
        {

            case ResourceType::SceneData: {
                return makeShared<SceneDataResource>(makeResourceName(resourceTemplate));
            }
            case ResourceType::Image: {
                if (resourceTemplate.usageRanges.size() > 1)
                {
                    return createAliasedImage(resourceTemplate);
                }
                return createImage(resourceTemplate);
            }
            case ResourceType::Buffer:
            case ResourceType::TopLevelAS:
            case ResourceType::Unknown:
            default: {
                throw std::runtime_error(std::format("[ResourceFactory] Error: ResourceType {} not implemented", toString(resourceTemplate.resourceType)));
            }
        }
    }

    std::string ResourceFactory::makeResourceName(const OptimizerResource& resourceTemplate) noexcept
    {
        return std::format("Res[{}-{}]", resourceTemplate.id, toString(resourceTemplate.resourceType));
    }

    SPtr<ImageResource> ResourceFactory::createImage(const OptimizerResource& resourceTemplate) const noexcept
    {
        if (resourceTemplate.resourceType != ResourceType::Image)
        {
            return nullptr;
        }

        const auto name = makeResourceName(resourceTemplate);

        const auto firstUse = resourceTemplate.getUsagePoint(resourceTemplate.usageRanges[0].start).value();
        auto*      params   = std::get_if<ImageInfo>(&(firstUse.dependencyInfo.resourceParams));

        const auto rhiImage = mRHI->createImage({
            .extent        = params->extent.value_or(mRHI->getSwapchain()->getProperties().extent),
            .format        = params->format,
            .usageFlags    = getImageUsageFlags(resourceTemplate),
            .createSampler = true,
            .aliased       = false,
            .debugName     = name,
        });

        return makeShared<ImageResource>(rhiImage, name);
    }

    SPtr<ImageResource> ResourceFactory::createAliasedImage(const OptimizerResource& resourceTemplate) const noexcept
    {
        if (resourceTemplate.resourceType != ResourceType::Image)
        {
            return nullptr;
        }

        std::vector<SPtr<RHI::Image>> rhiImages;
        for (const auto& usageRange : resourceTemplate.usageRanges)
        {
            const auto firstUse = resourceTemplate.getUsagePoint(usageRange.start).value();
            auto*      params   = std::get_if<ImageInfo>(&(firstUse.dependencyInfo.resourceParams));

            const auto name = std::format("Res[{}-{} | {}]", resourceTemplate.id, toString(resourceTemplate.resourceType), firstUse.usedAs);

            const auto rhiImage = mRHI->createImage({
                .extent        = params->extent.value_or(mRHI->getSwapchain()->getProperties().extent),
                .format        = params->format,
                .usageFlags    = getImageUsageFlags(resourceTemplate),
                .createSampler = true,
                .aliased       = false,
                .debugName     = name,
            });

            rhiImages.push_back(rhiImage);
        }

        return makeShared<ImageResource>(rhiImages, mRHI->allocatedAliasedImageMemory(rhiImages), makeResourceName(resourceTemplate));
    }

    vk::ImageUsageFlags ResourceFactory::getImageUsageFlags(const OptimizerResource& resourceTemplate)
    {
        if (resourceTemplate.resourceType != ResourceType::Image)
        {
            return {};
        }

        vk::ImageUsageFlags mergedUsageFlags;
        for (const auto& usageRange : resourceTemplate.usageRanges)
        {
            if (const auto firstUse = resourceTemplate.getUsagePoint(usageRange.start); firstUse.has_value())
            {
                const auto& resourceParams = firstUse->dependencyInfo.resourceParams;
                if (auto* params = std::get_if<ImageInfo>(&resourceParams))
                {
                    mergedUsageFlags |= RHI::getImageUsageFlags(params->imageUsage);
                }
            }
        }
        return mergedUsageFlags;
    }
}
