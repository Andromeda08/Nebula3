#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Commands/TrackedState.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/ImageTraits.hpp"

namespace RHI
{
    class CommandList;
    // =====================================
    // Image Class
    // =====================================

    struct RHIImageCreateInfo
    {
        vk::Extent2D        extent          = { 1280, 720 };
        vk::Format          format          = vk::Format::eR32G32B32A32Sfloat;
        vk::ImageUsageFlags usageFlags      = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
        vk::SampleCountFlagBits samples     = vk::SampleCountFlagBits::e1;
        bool                createSampler   = false;
        bool                aliased         = false;
        bool                mipmapping      = false;
        bool                cubeMap         = false;
        std::string         debugName       = "Unknown Image";

        std::optional<vk::SamplerCreateInfo> samplerInfo = std::nullopt;
    };

    struct ImageCreateInfo : public RHIImageCreateInfo
    {
        SPtr<Device> device;
    };

    struct SwapchainBackedImage
    {

    };

    struct AllocationBackedImage
    {
        bool               hasMemory      = false;
        VmaAllocation      allocation     = {};
        VmaAllocationInfo  allocationInfo = {};
    };

    using ImageUnderlyingResource = std::variant<std::monostate, AllocationBackedImage, SwapchainBackedImage>;

    class Image : public Resource
    {
    public:
        nbl_DISABLE_COPY(Image);
        nbl_CTOR_SHARED(Image);

        ~Image() override;

        [[nodiscard]] const vk::ImageView& getMipView(size_t i) const noexcept;

        void generateMipmaps(const CommandList* commandList, vk::Filter filter = vk::Filter::eNearest);

        [[nodiscard]] TrackedImageState& getState() { return mState; }

        [[nodiscard]] const vk::Image&        getImage()      const { return mImage; }
        [[nodiscard]] const vk::ImageView&    getImageView()  const { return mImageView; }
        [[nodiscard]] const ImageProperties&  getProperties() const { return mProperties; }

        [[deprecated("Per-image samplers are deprecated")]]
        [[nodiscard]] const vk::Sampler&      getSampler()    const { return mSampler; }

    private:
        vk::Image                   mImage;
        vk::ImageView               mImageView;
        std::vector<vk::ImageView>  mMipViews;

        // TODO: Remove these
        vk::Sampler                 mSampler;

        TrackedImageState           mState;
        const ImageProperties       mProperties;
    };
}