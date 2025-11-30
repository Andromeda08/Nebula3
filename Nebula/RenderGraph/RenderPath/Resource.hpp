#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "Core/Macro.hpp"
#include "RenderGraph/RenderGraphTraits.hpp"
#include "RenderGraph/Compiler/ResourceOptimizer.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace rg
{
    // Render Graph resource base class.
    class Resource
    {
    public:
        Resource(std::string name, ResourceType resourceType);

        virtual ~Resource() = default;

        template <class T>
        T* as()
        {
            static_assert(std::is_base_of_v<Resource, T>, "Template parameter T must be a valid Resource type");
            return dynamic_cast<T*>(this);
        }

        [[nodiscard]] const std::string& getName() const noexcept;

        ResourceType getResourceType() const noexcept;

    private:
        std::string     mName;
        ResourceType    mResourceType;
    };

    class SceneDataResource final : public Resource
    {
    public:
        nbl_DISABLE_COPY(SceneDataResource);
        ~SceneDataResource() override = default;

        explicit SceneDataResource(std::string name);
    };

    class ImageResource final : public Resource
    {
    public:
        nbl_DISABLE_COPY(ImageResource);
        ~ImageResource() override = default;

        ImageResource(const SPtr<RHI::Image>& image, std::string name);

        ImageResource(
            const std::vector<SPtr<RHI::Image>>& images,
            const RHI::Allocation&               allocation,
            std::string                          name);

        [[nodiscard]] SPtr<RHI::Image> getImage(uint32_t i = 0) const noexcept;

    private:
        std::vector<SPtr<RHI::Image>>   mImages;
        std::optional<RHI::Allocation>  mAliasedMemory = std::nullopt;
        const bool                      mIsAliased     = false;
    };

    class ResourceFactory
    {
    public:
        explicit ResourceFactory(const SPtr<RHI::VulkanRHI>& rhi);

        SPtr<Resource> create(const OptimizerResource& resourceTemplate) const;

    private:
        [[nodiscard]] static std::string makeResourceName(const OptimizerResource& resourceTemplate) noexcept;

        SPtr<ImageResource> createImage(const OptimizerResource& resourceTemplate) const noexcept;

        SPtr<ImageResource> createAliasedImage(const OptimizerResource& resourceTemplate) const noexcept;

        [[nodiscard]] static vk::ImageUsageFlags getImageUsageFlags(const OptimizerResource& resourceTemplate);

        SPtr<RHI::VulkanRHI> mRHI;
    };
}
