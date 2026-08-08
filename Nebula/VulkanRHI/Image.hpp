#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Detail/ImageTraits.hpp"
#include "Detail/Resource.hpp"

namespace RHI
{
    struct TrackedImageStateDetails
    {
        vk::AccessFlags2        access;
        vk::PipelineStageFlags2 stage;
        vk::ImageLayout         layout;
        Range                   mipRange;

        TrackedImageStateDetails& setRange(const Range& range)
        {
            mipRange = range;
            return *this;
        }

        [[nodiscard]] bool isEquivalentTo(const TrackedImageStateDetails& other) const
        {
            return access   == other.access
                && stage    == other.stage
                && layout   == other.layout;
        }

        [[nodiscard]] bool operator==(const TrackedImageStateDetails& other) const
        {
            return mipRange == other.mipRange
                && isEquivalentTo(other);
        }

        [[nodiscard]] std::string toString() const
        {
            return fmt::format("Tracked state for mip range: {}\n\t- Access: {}\n\t- Stages: {}\n\t- Layout: {}",
                mipRange.toString(), vk::to_string(access), vk::to_string(stage), vk::to_string(layout));
        }
    };

    class TrackedImageState
    {
    public:
        explicit TrackedImageState(
            const vk::Image& image,
            const ImageProperties& imageProperties,
            const vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined)
        : mImage(image)
        , mImageProperties(imageProperties)
        , mMipRange(Range(0, static_cast<int32_t>(imageProperties.levelCount) - 1))
        {
            mState.push_back({
                .access   = vk::AccessFlagBits2::eNone,
                .stage    = vk::PipelineStageFlagBits2::eNone,
                .layout   = initialLayout,
                .mipRange = mMipRange,
            });
        }

        [[nodiscard]] std::vector<vk::ImageMemoryBarrier2> generateBarriers(const ImageUsage usage, const uint32_t mipLevel)
        {
            return generateBarriers(getImageState(usage), Range(mipLevel, mipLevel));
        }

        [[nodiscard]] std::vector<vk::ImageMemoryBarrier2> generateBarriers(const ImageUsage usage, const Range& range)
        {
            return generateBarriers(getImageState(usage), range);
        }

        [[nodiscard]] std::vector<vk::ImageMemoryBarrier2> generateBarriers(const ImageState& dst, const Range& range)
        {
            std::vector<vk::ImageMemoryBarrier2> result;

            for (const auto& tracked : mState)
            {
                if (!tracked.mipRange.overlaps(range))
                {
                    continue;
                }

                // Intersection of tracked range and requested range
                const auto start = std::max(tracked.mipRange.start, range.start);
                const auto end   = std::min(tracked.mipRange.end, range.end);

                auto barrier = makeImageMemoryBarrier({ tracked.layout, tracked.access, tracked.stage }, dst)
                    .setImage(mImage)
                    .setSubresourceRange(mImageProperties.getSubresourceRange()
                        .setBaseMipLevel(start)
                        .setLevelCount(end - start + 1));

                result.push_back(barrier);
            }

            for (const auto& b : result)
            {
                apply(b);
            }

            return result;
        }

        void apply(const vk::ImageMemoryBarrier2& barrier, const bool simplify = true)
        {
            const auto mipRange = Range(
                static_cast<int32_t>(barrier.subresourceRange.baseMipLevel),
                static_cast<int32_t>(barrier.subresourceRange.baseMipLevel + barrier.subresourceRange.levelCount - 1));

            if (mipRange.start < mMipRange.start || mipRange.end > mMipRange.end)
            {
                spdlog::warn("Out of bounds mip range in barrier.");
                return;
            }

            for (auto i = static_cast<int32_t>(mState.size()) - 1; i >= 0; --i)
            {
                if (!mState[i].mipRange.overlaps(mipRange))
                {
                    continue;
                }

                const auto old = mState[i];
                mState.erase(mState.begin() + i);

                if (old.mipRange.start < mipRange.start)
                {
                    mState.push_back(TrackedImageStateDetails(old)
                        .setRange(Range(old.mipRange.start, mipRange.start - 1)));
                }
                if (old.mipRange.end > mipRange.end)
                {
                    mState.push_back(TrackedImageStateDetails(old)
                        .setRange(Range(mipRange.end + 1, old.mipRange.end)));
                }
            }

            // Add new tracked state
            mState.push_back({
                .access   = barrier.dstAccessMask,
                .stage    = barrier.dstStageMask,
                .layout   = barrier.newLayout,
                .mipRange = mipRange,
            });

             if (simplify)
             {
                 cleanup();
             }
        }

        [[nodiscard]] bool isValid() const
        {
            bool hasOverlap  = false;
            bool outOfBounds = false;
            std::stringstream sstr;
            for (size_t i = 0; i < mState.size(); i++)
            {
                auto& x = mState[i];
                for (size_t j = 0; j < mState.size(); j++)
                {
                    if (i == j)
                    {
                        continue;;
                    }
                    const auto& y = mState[j];
                    if (x.mipRange.overlaps(y.mipRange))
                    {
                        hasOverlap = true;
                        sstr << fmt::format("Overlapping range found in tracked state: {} and {}\n", x.mipRange.toString(), y.mipRange.toString());
                    }
                }

                if (x.mipRange.start < mMipRange.start || x.mipRange.end > mMipRange.end)
                {
                    outOfBounds = true;
                    sstr << fmt::format("Range {} is out of bounds for tracked mip range {}", x.mipRange.toString(), mMipRange.toString());
                }
            }

            if (hasOverlap || outOfBounds)
            {
                spdlog::warn("{}", sstr.str());
            }

            return !hasOverlap && !outOfBounds;
        }

        void cleanup()
        {
            // Try to merge equivalent ranges
            for (size_t i = 0; i < mState.size(); i++)
            {
                for (size_t j = mState.size() - 1; j > i; --j)
                {
                    if (mState[i].isEquivalentTo(mState[j]) && mState[i].mipRange.adjacentTo(mState[j].mipRange))
                    {
                        mState[i].mipRange.grow(mState[j].mipRange);
                        mState.erase(mState.begin() + j);
                    }
                }
            }
        }

        void print() const
        {
            for (const auto& s : mState)
            {
                std::println("{}", s.toString());
            }
        }

    private:
        friend class Image;

        const vk::Image&                      mImage;
        const ImageProperties&                mImageProperties;
        Range                                 mMipRange;
        std::vector<TrackedImageStateDetails> mState;
    };

    class CommandList;
    class Swapchain;

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
        SPtr<Device> device;
        Swapchain*   pSwapchain;
        uint32_t     imageIndex;
    };

    struct AllocationBackedImage
    {
        Allocation* pAlloc;
    };

    using ImageUnderlyingResource = std::variant<std::monostate, AllocationBackedImage, SwapchainBackedImage>;

    class Image : public Resource
    {
    public:
        nbl_DISABLE_COPY(Image);
        nbl_CTOR_SHARED(Image);

        Image(Swapchain* pSwapchain, uint32_t index, const SPtr<Device>& device);

        ~Image() override;

        [[nodiscard]] const vk::ImageView& getMipView(size_t i) const noexcept;

        void generateMipmaps(const CommandList* commandList, vk::Filter filter = vk::Filter::eNearest);

        [[nodiscard]] vk::ImageMemoryBarrier2 getBarrier(const ImageUsage dstUsage)
        {
            return mState.generateBarriers(dstUsage, Range(0, static_cast<int32_t>(mProperties.levelCount - 1)))[0];
        }

        [[nodiscard]] ImageState getState() const
        {
            return {
                .layout     = mState.mState[0].layout,
                .accessMask = mState.mState[0].access,
                .stageMask  = mState.mState[0].stage,
            };
        }

        [[nodiscard]] TrackedImageState& getTrackedState();

        [[nodiscard]] const vk::Image&        getImage()      const { return mImage; }
        [[nodiscard]] const vk::ImageView&    getImageView()  const { return mImageView; }
        [[nodiscard]] const ImageProperties&  getProperties() const { return mProperties; }
        [[nodiscard]] const vk::Sampler&      getSampler()    const { return mSampler; }

    private:
        vk::Image                   mImage;
        vk::ImageView               mImageView;
        std::vector<vk::ImageView>  mMipViews;

        // TODO: Remove these
        vk::Sampler                 mSampler;

        ImageUnderlyingResource     mUnderlyingResource;

        const ImageProperties       mProperties;
        TrackedImageState           mState;
    };
}
