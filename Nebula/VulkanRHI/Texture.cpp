#include "Texture.hpp"

namespace RHI
{
    namespace detail
    {
        [[nodiscard]] static TextureProperties makeTextureProperties(const TextureCreateInfo& createInfo) noexcept
        {
            return {
                .format      = createInfo.format,
                .extent      = createInfo.extent,
                .aspectFlags = getImageAspectFlags(createInfo.format),
                .levelCount  = (createInfo.mipmapping && createInfo.extent.depth == 1) ? getMipLevels({ createInfo.extent.width, createInfo.extent.height }) : 1,
                .sampleCount = createInfo.sampleCount,
                .type        = createInfo.extent.depth == 1 ? TextureType::Texture2D : TextureType::Texture3D,
            };
        }

        [[nodiscard]] static vk::ImageType getImageType(const TextureProperties& properties) noexcept
        {
            if (properties.extent.height == 1 && properties.extent.depth == 1)
            {
                return vk::ImageType::e1D;
            }
            if (properties.extent.depth == 1)
            {
                return vk::ImageType::e2D;
            }
            return vk::ImageType::e3D;
        }

        [[nodiscard]] static vk::ImageViewType getEquivalentImageViewType(const vk::ImageType imageType) noexcept
        {
            switch (imageType)
            {
                case vk::ImageType::e1D: return vk::ImageViewType::e1D;
                case vk::ImageType::e2D: return vk::ImageViewType::e2D;
                case vk::ImageType::e3D: return vk::ImageViewType::e3D;
            }
            std::unreachable();
        }
    }

    Texture::Texture(const TextureCreateInfo& createInfo)
    : Resource(createInfo.device)
    , mAliased(createInfo.aliasing)
    , mProperties(detail::makeTextureProperties(createInfo))
    {
        // Create Image
        auto imageCreateInfo = vk::ImageCreateInfo()
            .setFormat(mProperties.format)
            .setExtent(mProperties.extent)
            .setSamples(mProperties.sampleCount)
            .setMipLevels(mProperties.levelCount)
            .setUsage(createInfo.usageFlags)
            .setImageType(detail::getImageType(mProperties))
            .setArrayLayers(1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        if (mAliased)
        {
            imageCreateInfo.setFlags(vk::ImageCreateFlagBits::eAlias);
        }
        else
        {
            mAllocation = mDevice->allocateImage({
                .pHandle   = &mImage,
                .imageInfo = imageCreateInfo,
            });
        }

        mDevice->nameObject<vk::Image>({
            .debugName = mLabel,
            .handle    = mImage,
        });

        // Create ImageView
        const auto viewCreateInfo = vk::ImageViewCreateInfo()
            .setFormat(mProperties.format)
            .setImage(mImage)
            .setSubresourceRange({ mProperties.aspectFlags, 0, mProperties.levelCount, 0, 1 })
            .setViewType(detail::getEquivalentImageViewType(imageCreateInfo.imageType));

        mImageView = mDevice->getHandle().createImageView(viewCreateInfo);

        mDevice->nameObject<vk::ImageView>({
            .debugName = std::format("{}-View", mLabel),
            .handle    = mImageView,
        });
    }

    Texture::~Texture()
    {
        mDevice->getHandle().destroy(mImageView);
        if (!mAliased)
        {
            vmaDestroyImage(mDevice->getAllocator(), mImage, mAllocation->getAllocation());
        }
        else
        {
             mDevice->getHandle().destroy(mImage);
        }
    }

    vk::ImageMemoryBarrier2 Texture::getBarrier(const ImageUsage& dstUsage, const bool update) noexcept
    {
        const auto dstState = getImageState(dstUsage);
        const auto barrier = makeImageMemoryBarrier(mState, dstState)
            .setImage(mImage)
            .setSubresourceRange({ mProperties.aspectFlags, 0, mProperties.levelCount, 0, 1 });

        if (update)
        {
            mState = dstState;
        }

        return barrier;
    }

    void Texture::useAliasedAllocation(const SPtr<Allocation>& pAllocation) noexcept
    {
        nbl_ASSERT(pAllocation->allowAliasedUse(), "The specified Allocation does not allow aliased use!");
        nbl_ASSERT(pAllocation == nullptr, "This Image is already bound to an allocation!");
        mAllocation = pAllocation;
        mAllocation->bindAliasedImageMemory(mImage);
    }
}
