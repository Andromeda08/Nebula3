#include "ResourceRegistry.hpp"

#include "SceneResource.hpp"
#include "Texture2DResource.hpp"
#include "Texture3DResource.hpp"
#include "RenderGraph/RenderGraphContext.hpp"
#include "VulkanRHI/Texture.hpp"

namespace rg
{
    std::vector<SPtr<Resource>> ResourceRegistry::create(const OptimizerResource& resourceTemplate) noexcept
    {
        const auto name = std::format("Res[{}-{}]", resourceTemplate.id, toString(resourceTemplate.resourceType));

        switch (resourceTemplate.resourceType)
        {
            case ResourceType::SceneData: {
                mResources[name] = makeShared<SceneResource>(mContext->getActiveScene(), name);
                return { mResources.at(name) };
            }
            case ResourceType::Texture2D: {
                // Alias if there are more than one usage ranges for the optimizer resource
                if (resourceTemplate.usageRanges.size() > 1)
                {
                    return createAliasedTexture2D(resourceTemplate, name);
                }
                return { createTexture<Texture2DResource>(resourceTemplate, name) };
            }
            case ResourceType::Texture3D: {
                return { createTexture<Texture3DResource>(resourceTemplate, name) };
            }
            default: {}
        }

        nbl_ASSERT(false, "Unknown resource type: {}", toString(resourceTemplate.resourceType));
        std::unreachable();
    }

    std::vector<SPtr<Resource>> ResourceRegistry::createAliasedTexture2D(const OptimizerResource& resourceTemplate, const std::string& name) noexcept
    {
        const auto swapchainExtent = mRHI->getSwapchain()->getProperties().extent;
        const auto defaultExtent = vk::Extent3D().setWidth(swapchainExtent.width).setHeight(swapchainExtent.height).setDepth(1);

        std::vector<SPtr<RHI::Texture>> pTextures;
        std::set<std::string>           localNames;
        for (const auto& usageRange : resourceTemplate.usageRanges)
        {
            const std::string localName = std::format("{}-[{}, {}]", name, usageRange.start, usageRange.end);
            const UsagePoint  firstUse  = resourceTemplate.getUsagePoint(usageRange.start).value();
            const ImageInfo*  params    = std::get_if<ImageInfo>(&(firstUse.dependencyInfo.resourceParams));

            auto texture = mRHI->createTexture({
                .extent      = params->extent.value_or(defaultExtent),
                .format      = params->format,
                .usageFlags  = RHI::getImageUsageFlags(params->imageUsage),
                .sampleCount = vk::SampleCountFlagBits::e1,
                .mipmapping  = false,
                .aliasing    = true,
                .label       = localName,
            });

            mResources[localName] = makeShared<Texture2DResource>(texture, localName, true);
            pTextures.push_back(mResources.at(localName)->as<Texture2DResource>()->getTexture());
            localNames.insert(localName);
        }

        auto alloc = mRHI->getDevice()->allocateAliasedImageMemory({
            .textures = pTextures,
        });
        for (auto& texture : pTextures)
        {
            texture->useAliasedAllocation(alloc);
        }

        mAliasedMemory[alloc] = pTextures;

        return mResources
            | std::views::filter([&localNames](const auto& resource){ return localNames.contains(resource.first); })
            | std::views::values
            | std::ranges::to<std::vector>();
    }

    vk::ImageUsageFlags ResourceRegistry::getImageUsageFlags(const OptimizerResource& resourceTemplate) noexcept
    {
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
