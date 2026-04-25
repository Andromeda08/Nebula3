#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
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

    class Image
    {
    public:
        nbl_DISABLE_COPY(Image);
        nbl_CTOR_SHARED(Image);

        ~Image();

        const vk::ImageView& getMipView(size_t i) const noexcept;

        /**
         * Create an ImageMemoryBarrier to the dstState.
         * @param dstUsage
         * @param update Update the internally tracked state
         */
        vk::ImageMemoryBarrier2 getBarrier(const ImageUsage& dstUsage, const bool update = true) noexcept
        {
            const auto dstState = getImageState(dstUsage);
            const auto barrier = makeImageMemoryBarrier(mState, dstState)
                .setImage(mImage)
                .setSubresourceRange(mProperties.getSubresourceRange());

            if (update)
            {
                mState = dstState;
            }

            return barrier;
        }

        void generateMipmaps(const CommandList* commandList, vk::Filter filter = vk::Filter::eNearest);

        void updateState(const ImageState& imageState)
        {
            mState = imageState;
        }

        void useAllocation(VmaAllocation allocation, const VmaAllocationInfo& allocationInfo);

        const vk::Image&        getImage()      const { return mImage; }
        const vk::ImageView&    getImageView()  const { return mImageView; }
        const vk::Sampler&      getSampler()    const { return mSampler; }
        const ImageProperties&  getProperties() const { return mProperties; }
        ImageState              getState()      const { return mState; }

    private:
        vk::Image               mImage;
        vk::ImageView           mImageView;
        vk::Sampler             mSampler;
        ImageState              mState;

        std::vector<vk::ImageView> mMipViews;

        bool                    mHasMemory      = false;
        VmaAllocation           mAllocation     = {};
        VmaAllocationInfo       mAllocationInfo = {};

        SPtr<Device>            mDevice;

        const ImageProperties   mProperties;
        const std::string       mDebugName;
    };
}