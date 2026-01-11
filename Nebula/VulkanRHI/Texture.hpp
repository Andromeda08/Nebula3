#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Core/Macro.hpp"
#include "Detail/ImageTraits.hpp"
#include "Detail/Resource.hpp"

// TODO: TextureTraits.hpp (?)
namespace RHI
{
    enum class TextureType
    {
        Texture2D,
        Texture3D,
    };

    struct TextureProperties
    {
        vk::Format              format;
        vk::Extent3D            extent;
        vk::ImageAspectFlags    aspectFlags;
        uint32_t                levelCount;
        vk::SampleCountFlagBits sampleCount;
        TextureType             type;

        [[nodiscard]] bool is2D() const noexcept
        {
            return extent.depth == 1u;
        }

        [[nodiscard]] vk::Extent2D getExtent2D() const noexcept
        {
            return { extent.width, extent.height };
        }

        [[nodiscard]] vk::Extent3D getExtent3D() const noexcept
        {
            return extent;
        }
    };
}

namespace RHI
{
    struct RHITextureCreateInfo
    {
        vk::Extent3D            extent      = { 1280, 720, 1 };
        vk::Format              format      = vk::Format::eR32G32B32A32Sfloat;
        vk::ImageUsageFlags     usageFlags  = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
        bool                    mipmapping  = false;
        bool                    aliasing    = false;
        std::string_view        label;
    };

    struct TextureCreateInfo : RHITextureCreateInfo
    {
        SPtr<Device> device;
    };

    class Texture : public Resource
    {
    public:
        nbl_DISABLE_COPY(Texture);
        nbl_CTOR_SHARED(Texture);

        ~Texture() override;

        /**
         * Create an ImageMemoryBarrier to the dstState.
         * @param dstUsage
         * @param update Update the internally tracked state
         */
        vk::ImageMemoryBarrier2 getBarrier(const ImageUsage& dstUsage, bool update = true) noexcept;

        void useAliasedAllocation(const SPtr<Allocation>& pAllocation) noexcept;

        [[nodiscard]] const vk::Image& getHandle() const noexcept
        {
            return mImage;
        }

        [[nodiscard]] const vk::ImageView& getImageView() const noexcept
        {
            return mImageView;
        }

        [[nodiscard]] const TextureProperties& getProperties() const noexcept
        {
            return mProperties;
        }

    private:
        vk::Image               mImage;
        vk::ImageView           mImageView;
        SPtr<Allocation>        mAllocation;
        ImageState              mState;
        const bool              mAliased;
        const TextureProperties mProperties;
    };
}