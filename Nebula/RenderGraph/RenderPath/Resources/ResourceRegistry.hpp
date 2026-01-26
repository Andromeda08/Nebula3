#pragma once

#include <map>
#include <vector>

#include "RenderGraph/RenderGraphContext.hpp"
#include "RenderGraph/RenderPath/Resource.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace rg
{
    class Texture3DResource;
    class Texture2DResource;

    class ResourceRegistry
    {
    public:
        ResourceRegistry(const SPtr<RenderGraphContext>& ctx, const SPtr<RHI::VulkanRHI>& rhi)
        : mContext(ctx)
        , mRHI(rhi)
        {
        }

        /**
         * Create and allocate the underlying RHI resource for a resource template.
         * @return List of created resources (size>1 if aliased).
         */
        std::vector<SPtr<Resource>> create(const OptimizerResource& resourceTemplate) noexcept;

    private:
        /**
         * Create and allocate a non-aliased Texture (2D or 3D) from an OptimizerResource.
         * @tparam T Texture resource type
         * @param resourceTemplate
         * @param name
         * @return SPtr to allocated Resource
         */
        template <class T>
        [[nodiscard]] SPtr<Resource> createTexture(const OptimizerResource& resourceTemplate, const std::string& name) noexcept
        {
            const auto swapchainExtent = mRHI->getSwapchain()->getProperties().extent;
            const auto defaultExtent = vk::Extent3D().setWidth(swapchainExtent.width).setHeight(swapchainExtent.height).setDepth(1);

            const auto firstUse = resourceTemplate.getUsagePoint(resourceTemplate.usageRanges[0].start).value();
            auto*      params   = std::get_if<ImageInfo>(&(firstUse.dependencyInfo.resourceParams));

            auto texture = mRHI->createTexture({
                .extent      = params->extent.value_or(defaultExtent),
                .format      = params->format,
                .usageFlags  = getImageUsageFlags(resourceTemplate),
                .sampleCount = vk::SampleCountFlagBits::e1,
                .mipmapping  = false,
                .aliasing    = false,
                .label       = name,
            });

            if constexpr (std::is_same_v<Texture2DResource, T>)
            {
                mResources[name] = makeShared<T>(texture, name, false);
            }
            else if constexpr (std::is_same_v<Texture3DResource, T>)
            {
                mResources[name] = makeShared<T>(texture, name);
            }
            else
            {
                nbl_ASSERT(false, "Unknown Texture type!");
            }

            return mResources.at(name);
        }

        /**
         * Create logical Texture2D-s and allocate aliased memory for them from an OptimizerResource.
         * - Aliasing is possible if the OptimizerResource contains multiple "UsageRanges" i.e. different usages of a Texture don't overlap in time.
         * - Creates multiple RHI Textures and binds a single allocation to all of them.
         * - Naming scheme: Res[{id}-{resourceType}]-[{usageRangeStart}, [usageRangeEnd]]
         * @param resourceTemplate
         * @param name
         * @return List of resources created (that share an allocation)
         */
        [[nodiscard]] std::vector<SPtr<Resource>> createAliasedTexture2D(const OptimizerResource& resourceTemplate, const std::string& name) noexcept;

        /**
         * Infers RHI image usage flag bits from ImageUsage.
         * @param resourceTemplate
         * @return
         */
        [[nodiscard]] static vk::ImageUsageFlags getImageUsageFlags(const OptimizerResource& resourceTemplate) noexcept;

        std::map<std::string, SPtr<Resource>>                            mResources;
        std::map<SPtr<RHI::Allocation>, std::vector<SPtr<RHI::Texture>>> mAliasedMemory;
        SPtr<RenderGraphContext>                                         mContext;
        SPtr<RHI::VulkanRHI>                                             mRHI;
    };
}
