#pragma once

#include "Common.hpp"

/**
 *
 */
namespace sunflower::rhi
{
    template <class To, class From>
    requires std::derived_from<To, From>
    [[nodiscard]] To* rhi_cast(From* ptr) noexcept
    {
        if constexpr (conf::gIsDebug)
        {
            auto* result = dynamic_cast<To*>(ptr);
            if (!result)
            {
                ::sunflower::exit("RHI type mismatch!");
            }
            return result;
        }
        else
        {
            return static_cast<To*>(ptr);
        }
    }

    template <class To, class From>
    requires std::derived_from<To, From>
    [[nodiscard]] const To* rhi_cast(const From* ptr) noexcept
    {
        if constexpr (conf::gIsDebug)
        {
            auto* result = dynamic_cast<const To*>(ptr);
            if (!result)
            {
                ::sunflower::exit("RHI type mismatch!");
            }
            return result;
        }
        else
        {
            return static_cast<const To*>(ptr);
        }
    }

    struct ICommandList;
    struct Texture;

    struct FrameInfo
    {
        uint64_t currentFrameIndex;
        uint64_t frameNumber;
        uint64_t frameValue;
        uint32_t acquiredIndex;
        Texture* pSwapchainTexture;
    };

    struct PresentFrameInfo
    {
        const FrameInfo frameInfo;
        ICommandList*   pCommandList;
    };

    struct TimelineSync
    {
        sunflower_INTERFACE(TimelineSync);

        virtual uint64_t getSignaledValue() const = 0;

        virtual bool hostWait(uint64_t value, uint64_t timeout) const = 0;

        virtual void hostSignal(uint64_t value) const = 0;
    };

    struct TimelineSyncPoint
    {
        TimelineSync*   pSync = nullptr;
        uint64_t        value = 0;
    };

    struct SubmitInfo
    {
        std::vector<ICommandList*>     commandLists;
        std::vector<TimelineSyncPoint> waits;
        std::vector<TimelineSyncPoint> signals;

        SubmitInfo& addCommandList(ICommandList* pList)
        {
            commandLists.push_back(pList);
            return *this;
        }

        SubmitInfo& addWait(TimelineSync* pSync, const uint64_t value)
        {
            waits.push_back({
                .pSync = pSync,
                .value = value,
            });
            return *this;
        }

        SubmitInfo& addSignal(TimelineSync* pSync, const uint64_t value)
        {
            signals.push_back({
                .pSync = pSync,
                .value = value,
            });
            return *this;
        }
    };

    class Timeline
    {
    public:
        sunflower_DisableCopy(Timeline);

        explicit Timeline(UPtr<TimelineSync> sync) : mSync(std::move(sync)) {}

        [[nodiscard]] uint64_t getNextValue() noexcept
        {
            return ++mLast;
        }

        [[nodiscard]] uint64_t getLastValue() const noexcept
        {
            return mLast;
        }

        [[nodiscard]] bool isComplete(const uint64_t value) const
        {
            return mSync->getSignaledValue() >= value;
        }

        [[nodiscard]] bool hostWait(const uint64_t value, const uint64_t timeout = std::numeric_limits<uint64_t>::max()) const
        {
            return mSync->hostWait(value, timeout);
        }

        [[nodiscard]] TimelineSync* getSync() const noexcept
        {
            return mSync.get();
        }

        [[nodiscard]] TimelineSyncPoint makePoint(const uint64_t value) const noexcept
        {
            return{
                .pSync = mSync.get(),
                .value = value,
            };
        }

    private:
        UPtr<TimelineSync>  mSync;
        uint64_t            mLast = 0;
    };

    struct ICommandList
    {
        sunflower_INTERFACE(ICommandList);

        virtual void begin() = 0;
        virtual void end() = 0;
    };

    struct ICommandPool
    {
        sunflower_INTERFACE(ICommandPool);

        virtual ICommandList* allocate() = 0;

        virtual void free(const ICommandList* pCommandList) = 0;

        virtual void reset() = 0;
    };

    struct ICommandQueue
    {
        sunflower_INTERFACE(ICommandQueue);

        [[nodiscard]] virtual Timeline* getTimeline() const = 0;

        [[nodiscard]] virtual QueueType getQueueType() const noexcept = 0;

        [[nodiscard]] virtual UPtr<ICommandPool> createCommandPool() = 0;

        virtual void waitIdle() const = 0;

        virtual void submit(const SubmitInfo& submitInfo) const = 0;

        virtual void immediate(const std::function<void(ICommandList*)>& fn) const = 0;
    };

    struct Resource
    {
        sunflower_INTERFACE(Resource);
    };

    struct PendingDelete
    {
        UPtr<Resource> resource;
        uint64_t       deletionFrame;
    };

    enum class Format : std::uint16_t
    {
        None,
        RGBA8_Unorm,
        RGBA8_Srgb,
        BGRA8_Unorm,
        BGRA8_Srgb,
        R32_Float,
        RG32_Float,
        RGBA16_Float,
        RGBA32_Float,
        D32_Float,
    };

    enum class TextureUsage : std::uint32_t
    {
        None        = 0,
        Sampled     = 1u << 0u,
        Storage     = 1u << 1u,
        ColorTarget = 1u << 2u,
        DepthTarget = 1u << 3u,
        TransferSrc = 1u << 4u,
        TransferDst = 1u << 5u,
    };
    sunflower_BasicFlags(TextureUsage);

    enum class TextureDescriptorType : std::uint8_t
    {
        Sampled,
        Storage,
    };

    enum class TextureType : std::uint8_t
    {
        e1D,
        e2D,
        e3D,
        eCube,
    };

    struct TextureCreateInfo
    {
        // Computes the number of mip levels for the specified size.
        static constexpr auto sMipLevelsMax = 0u;

        Size            size      = {};
        std::uint32_t   mipLevels = 1;
        Format          format    = Format::RGBA8_Unorm;
        TextureUsage    usage     = TextureUsage::Sampled;
        TextureType     type      = TextureType::e2D;
        Option<String>  label     = std::nullopt;
    };

    struct Texture : Resource
    {
        sunflower_INTERFACE(Texture);

        virtual Format getFormat() const noexcept = 0;

        virtual Size getSize() const noexcept = 0;
    };

    /**
     * Gets the mip levels based on the Texture specifications.
     * @note Validates the following in debug mode:
     * - Mip levels of non-2D and multisampled images must equal 1.
     * - The user specified mip levels can't exceed the levels that the given size can accommodate.
     */
    [[nodiscard]] std::uint32_t getMipLevels(const TextureCreateInfo& createInfo, bool isMultisampled = false);

    [[nodiscard]] std::uint32_t getTextureLayerCount(const TextureCreateInfo& createInfo) noexcept;

    [[nodiscard]] Size validateTextureSize(const Size& size, TextureType textureType);

    inline constexpr auto gSwapchainFormat = Format::BGRA8_Unorm;

    struct Swapchain
    {
        sunflower_INTERFACE(Swapchain);

        virtual Texture* getTexture(uint64_t i) const = 0;

        virtual Format getFormat() const noexcept = 0;

        virtual Size getSize() const noexcept = 0;
    };

    struct DynamicRHI
    {
        sunflower_INTERFACE(DynamicRHI);

        // Acquire the next swapchain image and begin a new frame.
        [[nodiscard]] virtual FrameInfo beginFrame() = 0;

        // Submit commands and present the Frame described by a FrameInfo returned at begin time.
        virtual void endFrame_submitAndPresent(const PresentFrameInfo& presentFrameInfo) = 0;

        // Checks if a frame (by number) has been completed.
        [[nodiscard]] virtual bool isFrameComplete(uint64_t frame) const = 0;

        // Queue a resource for deletion.
        virtual void queueDeletion(UPtr<Resource> resource) = 0;

        // Get the Graphics (with present support) queue.
        virtual ICommandQueue* getGraphicsQueue() const = 0;

        // Note: Async compute queues are optional!
        virtual ICommandQueue* getComputeQueue() const = 0;

        [[nodiscard]] virtual UPtr<Texture> createTexture(const TextureCreateInfo& textureInfo) = 0;
    };
}
