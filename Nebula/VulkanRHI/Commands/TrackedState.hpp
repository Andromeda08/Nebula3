#pragma once

#include <print>
#include <ranges>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <vulkan/vulkan.hpp>

#include "Core/Types.hpp"
#include "Core/Util.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Detail/ImageTraits.hpp"

namespace RHI
{
    struct BufferMemoryBarrier
    {
        BufferState dstState;
        Buffer*     pBuffer;
    };

    struct ImageMemoryBarrier
    {
        ImageState dstState;
        Image*     pImage;
        Range      range;
    };
    
    struct Barrier2
    {
        Barrier2& addImageBarrier(Image* pImage, const ImageState& dstState, const std::optional<Range>& mipRange)
        {
            imageBarriers.push_back({ dstState, pImage, mipRange.value_or(Range(0, pImage->getProperties().levelCount - 1)) });
            return *this;
        }

        Barrier2& addImageBarrier(Image* pImage, const ImageUsage& dstUsage, const std::optional<Range>& mipRange)
        {
            return addImageBarrier(pImage, getImageState(dstUsage), mipRange);
        }

        Barrier2& addImageBarrier(Image* pImage, const ImageUsage& dstUsage, const uint32_t mipLevel)
        {
            return addImageBarrier(pImage, getImageState(dstUsage), Range(mipLevel, mipLevel));
        }

        Barrier2& addBufferBarrier(Buffer* pBuffer, const BufferState& dstState)
        {
            bufferBarriers.push_back({ dstState, pBuffer });
            return *this;
        }

        Barrier2& addBufferBarrier(Buffer* pBuffer, const BufferUsage& dstUsage)
        {
            return addBufferBarrier(pBuffer, getBarrierFlagsForBufferUsage(dstUsage));
        }

        std::vector<ImageMemoryBarrier>  imageBarriers;
        std::vector<BufferMemoryBarrier> bufferBarriers;
    };

    struct TrackedBufferStateDetails
    {
        vk::AccessFlags2        access;
        vk::PipelineStageFlags2 stage;

        [[nodiscard]] bool isEquivalentTo(const TrackedBufferStateDetails& other) const
        {
            return access   == other.access
                && stage    == other.stage;
        }

        [[nodiscard]] std::string toString() const
        {
            return fmt::format("Tracked Buffer State:\n\t- Access: {}\n\t- Stages: {}", vk::to_string(access), vk::to_string(stage));
        }
    };

    class TrackedBufferState
    {
    public:
        TrackedBufferState()
        {
            mState = {
                .access   = vk::AccessFlagBits2::eNone,
                .stage    = vk::PipelineStageFlagBits2::eNone,
            };
        }

        [[nodiscard]] vk::BufferMemoryBarrier2 getBarrier(const Buffer* pBuffer, const BufferState& dstState)
        {
            const auto b = vk::BufferMemoryBarrier2()
                .setBuffer(pBuffer->getHandle())
                .setSize(VK_WHOLE_SIZE)
                .setSrcAccessMask(mState.access)
                .setDstAccessMask(dstState.access)
                .setSrcStageMask(mState.stage)
                .setDstStageMask(dstState.stage);

            apply(b);

            return b;
        }

        void apply(const vk::BufferMemoryBarrier2& barrier)
        {
            mState.access = barrier.dstAccessMask;
            mState.stage  = barrier.dstStageMask;
        }

        void print() const
        {
            std::println("{}", mState.toString());
        }

    private:
        TrackedBufferStateDetails mState;
    };

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
        explicit TrackedImageState(const Range& mipRange)
        : mMipRange(mipRange)
        {
            mState.push_back({
                .access   = vk::AccessFlagBits2::eNone,
                .stage    = vk::PipelineStageFlagBits2::eNone,
                .layout   = vk::ImageLayout::eUndefined,
                .mipRange = mipRange,
            });
        }

        [[nodiscard]] std::vector<vk::ImageMemoryBarrier2> generateBarriers(const Image* pImage, const ImageUsage usage, const Range& range)
        {
            return generateBarriers(pImage, getImageState(usage), range);
        }

        [[nodiscard]] std::vector<vk::ImageMemoryBarrier2> generateBarriers(const Image* pImage, const ImageState& dst, const Range& range)
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
                    .setImage(pImage->getImage())
                    .setSubresourceRange(pImage->getProperties().getSubresourceRange()
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
        Range                                 mMipRange;
        std::vector<TrackedImageStateDetails> mState;
    };
}
