#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Core/Macro.hpp"
#include "Core/Platform.hpp"
#include "Core/Types.hpp"

#include "VulkanRHI/Buffer.hpp"

// RHI Flags
// ================================
namespace RHI
{
    template <typename BitType>
    struct IsFlagType : std::false_type {};

    // Preventing conflicts with Vulkan flag types
    #ifdef nbl_VulkanRHI
    template <typename T>
    struct IsVulkanFlagType : std::false_type {};

    template <typename T>
        requires requires { typename vk::FlagTraits<T>; }
    struct IsVulkanFlagType<T> : std::bool_constant<vk::FlagTraits<T>::isBitmask> {};
    #endif

    template <typename BitType>
    class Flags
    {
    public:
        using MaskType = std::underlying_type_t<BitType>;

        constexpr          Flags()              : mMask(0) {}
        constexpr          Flags(BitType bit)   : mMask(static_cast<MaskType>(bit)) {}
        constexpr explicit Flags(MaskType mask) : mMask(mask) {}

        constexpr Flags operator|(Flags rhs) const noexcept { return Flags(mMask | rhs.mMask); }
        constexpr Flags operator&(Flags rhs) const noexcept { return Flags(mMask & rhs.mMask); }
        constexpr Flags operator^(Flags rhs) const noexcept { return Flags(mMask ^ rhs.mMask); }
        constexpr Flags operator~()          const noexcept { return Flags(~mMask); }

        Flags& operator|=(Flags rhs) noexcept { mMask |= rhs.mMask; return *this; }
        Flags& operator&=(Flags rhs) noexcept { mMask &= rhs.mMask; return *this; }
        Flags& operator^=(Flags rhs) noexcept { mMask ^= rhs.mMask; return *this; }

        constexpr bool operator==(Flags rhs) const noexcept { return mMask == rhs.mMask; }
        constexpr bool operator!=(Flags rhs) const noexcept { return mMask != rhs.mMask; }

        constexpr explicit operator MaskType() const noexcept { return mMask; }

        constexpr bool contains(BitType bit) const noexcept
        {
            const auto maskBit = static_cast<MaskType>(bit);
            return (mMask & maskBit) == maskBit;
        }

    private:
        MaskType mMask;
    };

    // RHI Flag Type concept
    #ifdef nbl_VulkanRHI
    template <typename T>
    concept RHIFlagType = IsFlagType<T>::value && !IsVulkanFlagType<T>::value;
    #else
    template <typename T>
    concept RHIFlagType = IsFlagType<T>::value;
    #endif

    template <RHIFlagType BitType>
    constexpr Flags<BitType> operator|(BitType lhs, BitType rhs)
    {
        return Flags<BitType>(lhs) | rhs;
    }

    template <RHIFlagType BitType>
    constexpr Flags<BitType> operator&(BitType lhs, BitType rhs)
    {
        return Flags<BitType>(lhs) & rhs;
    }

    template <RHIFlagType BitType>
    constexpr Flags<BitType> operator^(BitType lhs, BitType rhs)
    {
        return Flags<BitType>(lhs) ^ rhs;
    }
}

#define nbl_DECL_FLAG_TYPE(Name, BitType)                       \
    template <> struct IsFlagType<BitType> : std::true_type {}; \
    using Name = Flags<BitType>;

// Conversion functions are only available when the backend specific macro is defined.

// Vulkan Specifics
// - As methods on objects  :   .vk( )
// - As standalone functions: to_vk(T)
// ================================
#ifdef nbl_VulkanRHI
    #include <vulkan/vulkan.hpp>
#endif

// Metal Specifics
// - As methods on objects  :   .mtl( )
// - As standalone functions: to_mtl(T)
// ================================
#ifdef nbl_MetalRHI
    #include <metal/metal.hpp>

    template <class T>
    using NSPtr = NS::SharedPtr<T>;
#endif

// Forward Declarations
// ================================
namespace RHI
{
    enum class Backend;
    enum class ImageUsage;
    enum class Format;

    // Resources
    class Buffer;
    class Descriptor;
    class Image;
    class Pipeline;
    class RenderPass;

    // Commands
    class Barrier;
    class CommandPool;
    class CommandQueue;

    // WSI
    class IWindow;
}

// Promoted from RHI2 to RHI
// ================================
namespace RHI
{
    struct FrameData
    {
        vk::Fence       waitFence;
        vk::Semaphore   imageReadySemaphore;
        vk::Semaphore   renderingFinishedSemaphore;
        const uint64_t  currentFrame;
        const uint32_t  acquiredIndex;
    };

    struct PresentSubmitInfo
    {
        const FrameData     frameData;
        class CommandList*  pCommandList;
    };

    // Basic Buffer to Image copy operation
    struct BufferImageCopyInfo
    {
        Buffer* pSrcBuffer;
        Image*  pDstImage;
    };
}

// New RHI
// ================================
namespace RHI
{
    enum class Backend
    {
        Vulkan,
        Metal,
    };

    struct RHICreateInfo
    {
        Backend       backend;
        SPtr<IWindow> window;
    };

    using DeviceSize = std::uint64_t;

    /**
     * Define a buffer copy operation.
     * (equivalent to VkBufferCopy2)
     */
    struct BufferRegion
    {
        DeviceSize srcOffset = 0;
        DeviceSize dstOffset = 0;
        DeviceSize size      = 0;

        BufferRegion& setSrcOffset(const DeviceSize value) noexcept
        {
            srcOffset = value;
            return *this;
        }

        BufferRegion& setDstOffset(const DeviceSize value) noexcept
        {
            dstOffset = value;
            return *this;
        }

        BufferRegion& setSize(const DeviceSize value) noexcept
        {
            size = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::BufferCopy2 vk() const noexcept
        {
            return { srcOffset, dstOffset, size };
        }
        #endif
    };

    /**
     * Define a buffer copy command
     * (equivalent to VkCopyBufferInfo2)
     */
    struct CopyBufferInfo
    {
        Buffer*                   srcBuffer = nullptr;
        Buffer*                   dstBuffer = nullptr;
        std::vector<BufferRegion> regions   = {};

        CopyBufferInfo& setSrcBuffer(Buffer* pBuffer) noexcept
        {
            srcBuffer = pBuffer;
            return *this;
        }

        CopyBufferInfo& setDstBuffer(Buffer* pBuffer) noexcept
        {
            dstBuffer = pBuffer;
            return *this;
        }

        CopyBufferInfo& setRegions(const BufferRegion& region) noexcept
        {
            regions = { region };
            return *this;
        }

        CopyBufferInfo& setRegions(const std::vector<BufferRegion>& value) noexcept
        {
            regions = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::CopyBufferInfo2 vk() const noexcept
        {
            const auto r = regions
                | std::views::transform([](const auto& region){ return region.vk(); })
                | std::ranges::to<std::vector>();
            return vk::CopyBufferInfo2()
                .setSrcBuffer(srcBuffer->getHandle())
                .setDstBuffer(dstBuffer->getHandle())
                .setRegions(r);
        }
        #endif
    };

    struct Extent2D
    {
        uint32_t width  = 0;
        uint32_t height = 0;

        Extent2D& setWidth(const uint32_t value) noexcept
        {
            width = value;
            return *this;
        }

        Extent2D& setHeight(const uint32_t value) noexcept
        {
            height = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Extent2D vk() const noexcept
        {
            return { width, height };
        }
        #endif

        #ifdef nbl_MetalRHI
        [[nodiscard]] MTL::Size mtl() const noexcept
        {
            return { width, height, 1 };
        }
        #endif
    };

    struct Extent3D
    {
        uint32_t width  = 0;
        uint32_t height = 0;
        uint32_t depth  = 0;

        Extent3D& setWidth(const uint32_t value) noexcept
        {
            width = value;
            return *this;
        }

        Extent3D& setHeight(const uint32_t value) noexcept
        {
            height = value;
            return *this;
        }

        Extent3D& setDepth(const uint32_t value) noexcept
        {
            depth = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Extent3D vk() const noexcept
        {
            return { width, height, depth };
        }
        #endif

        #ifdef nbl_MetalRHI
        [[nodiscard]] MTL::Size mtl() const noexcept
        {
            return { width, height, depth };
        }
        #endif
    };

    struct Offset2D
    {
        int32_t x = 0;
        int32_t y = 0;

        Offset2D& setX(const int32_t value) noexcept
        {
            x = value;
            return *this;
        }

        Offset2D& setY(const int32_t value) noexcept
        {
            y = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Offset2D vk() const noexcept
        {
            return { x, y };
        }
        #endif

        #ifdef nbl_MetalRHI
        // Note: Metal does not support negative values for offsets
        [[nodiscard]] MTL::Origin mtl() const noexcept
        {
            return { static_cast<uint32_t>(x), static_cast<uint32_t>(y), 0 };
        }
        #endif
    };

    struct Offset3D
    {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;

        Offset3D& setX(const int32_t value) noexcept
        {
            x = value;
            return *this;
        }

        Offset3D& setY(const int32_t value) noexcept
        {
            y = value;
            return *this;
        }

        Offset3D& setZ(const int32_t value) noexcept
        {
            z = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Offset3D vk() const noexcept
        {
            return { x, y, z };
        }
        #endif

        #ifdef nbl_MetalRHI
        // Note: Metal does not support negative values for offsets
        [[nodiscard]] MTL::Origin mtl() const noexcept
        {
            return { static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(z) };
        }
        #endif

    };

    struct Rect2D
    {
        Offset2D offset = {};
        Extent2D extent = {};

        Rect2D& setOffset(const Offset2D& value) noexcept
        {
            offset = value;
            return *this;
        }

        Rect2D& setExtent(const Extent2D& value) noexcept
        {
            extent = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Rect2D vk() const noexcept
        {
            return { offset.vk(), extent.vk() };
        }
        #endif

        #ifdef nbl_MetalRHI
        [[nodiscard]] MTL::ScissorRect mtl() const noexcept
        {
            const auto [x, y, z] = offset.mtl();
            return { x, y, extent.width, extent.height };
        }
        #endif
    };

    struct Viewport
    {
        float x        = 0.0f;
        float y        = 0.0f;
        float width    = 0.0f;
        float height   = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        Viewport& setX(const float value) noexcept
        {
            x = value;
            return *this;
        }

        Viewport& setY(const float value) noexcept
        {
            y = value;
            return *this;
        }

        Viewport& setWidth(const float value) noexcept
        {
            width = value;
            return *this;
        }

        Viewport& setHeight(const float value) noexcept
        {
            height = value;
            return *this;
        }

        Viewport& setMinDepth(const float value) noexcept
        {
            minDepth = value;
            return *this;
        }

        Viewport& setMaxDepth(const float value) noexcept
        {
            maxDepth = value;
            return *this;
        }

        #ifdef nbl_VulkanRHI
        [[nodiscard]] vk::Viewport vk() const noexcept
        {
            return { x, y, width, height, minDepth, maxDepth };
        }
        #endif

        #ifdef nbl_MetalRHI
        [[nodiscard]] MTL::Viewport mtl() const noexcept
        {
            return {
                static_cast<double>(x),
                static_cast<double>(y),
                static_cast<double>(width),
                static_cast<double>(height),
                static_cast<double>(minDepth),
                static_cast<double>(maxDepth),
            };
        }
        #endif
    };

    enum class IndexType
    {
        Uint16,
        Uint32,
    };

    #ifdef nbl_VulkanRHI
    [[nodiscard]] constexpr vk::IndexType to_vk(const IndexType indexType) noexcept
    {
        using enum IndexType;
        switch (indexType)
        {
            case Uint16: return vk::IndexType::eUint16;
            case Uint32: return vk::IndexType::eUint32;
        }
        exitWithError("Unsupported IndexType!");
    }
    #endif

    #ifdef nbl_MetalRHI
    [[nodiscard]] constexpr MTL::IndexType to_mtl(const IndexType indexType) noexcept
    {
        using enum IndexType;
        switch (indexType)
        {
            case Uint16: return MTL::IndexTypeUInt16;
            case Uint32: return MTL::IndexTypeUInt32;
        }
        exitWithError("Unsupported IndexType!");
    }
    #endif

    enum class Format
    {
        RGBA32f,
        RGB32f,
        RG32f,
        R32f,
        BGRA8Unorm,
        D32f,
    };

    [[nodiscard]] constexpr bool isColor(const Format format) noexcept
    {
        using enum Format;
        switch (format)
        {
            case RGBA32f:
            case RGB32f:
            case RG32f:
            case R32f:
            case BGRA8Unorm:
                return true;
            case D32f:
                return false;
        }
        exitWithError("Unhandled format!");
    }

    [[nodiscard]] constexpr bool hasDepthComponent(const Format format) noexcept
    {
        using enum Format;
        switch (format)
        {
            case RGBA32f:
            case RGB32f:
            case RG32f:
            case R32f:
            case BGRA8Unorm:
                return false;
            case D32f:
                return true;
        }
        exitWithError("Unhandled format!");
    }

    #ifdef nbl_VulkanRHI
    [[nodiscard]] constexpr vk::Format to_vk(const Format format) noexcept
    {
        using enum Format;
        switch (format)
        {
            case RGBA32f:       return vk::Format::eR32G32B32A32Sfloat;
            case RGB32f:        return vk::Format::eR32G32B32Sfloat;
            case RG32f:         return vk::Format::eR32G32Sfloat;
            case R32f:          return vk::Format::eR32Sfloat;
            case BGRA8Unorm:    return vk::Format::eB8G8R8A8Unorm;
            case D32f:          return vk::Format::eD32Sfloat;
        }
        exitWithError("Unsupported Format!");
    }
    #endif

    #ifdef nbl_MetalRHI
    [[nodiscard]] constexpr MTL::PixelFormat to_mtl(const Format format) noexcept
    {
        using enum Format;
        switch (format)
        {
            case RGBA32f:       return MTL::PixelFormatRGBA32Float;
            case RG32f:         return MTL::PixelFormatRG32Float;
            case R32f:          return MTL::PixelFormatR32Float;
            case BGRA8Unorm:    return MTL::PixelFormatBGRA8Unorm_sRGB;
            case D32f:          return MTL::PixelFormatDepth32Float;

            case RGB32f:
            {
                spdlog::warn("Metal does not support 3-component 32-bit float pixel formats. (using RGBA32)");
                return MTL::PixelFormatRGBA32Float;
            }
        }
        exitWithError("Unsupported Format!");
    }
    #endif

    enum class PresentMode
    {
        Fifo,
        Immediate,
        Mailbox,
    };

    #ifdef nbl_VulkanRHI
    [[nodiscard]] constexpr vk::PresentModeKHR to_vk(const PresentMode presentMode) noexcept
    {
        using enum PresentMode;
        switch (presentMode)
        {
            case Fifo:      return vk::PresentModeKHR::eFifo;
            case Immediate: return vk::PresentModeKHR::eImmediate;
            case Mailbox:   return vk::PresentModeKHR::eMailbox;
        }
        exitWithError("Unsupported PresentMode!");
    }
    #endif

    #ifdef nbl_MetalRHI
    // Present Mode for Metal is set via <CAMetalLayer.displaySyncEnabled>
    [[nodiscard]] constexpr bool getMTLDisplaySyncMode(const PresentMode presentMode) noexcept
    {
        return presentMode == PresentMode::Fifo;
    }
    #endif

    // ================================
    // Resource States, Synchronization
    // ================================

    struct ImageMemoryBarrier
    {
        Image*      pImage;
        ImageUsage  dstUsage;
    };

    /**
     * Specify dependency information for a synchronization command.
     * (roughly equivalent to VkDependencyInfo)
     */
    struct DependencyInfo
    {
        std::vector<ImageMemoryBarrier> imageMemoryBarriers;

        DependencyInfo& addBarrier(const ImageMemoryBarrier& imageMemoryBarrier) noexcept
        {
            imageMemoryBarriers.push_back(imageMemoryBarrier);
            return *this;
        }
    };

}

// Buffer Interface
// ================================
namespace RHI
{
    enum class BufferUsageFlagBits : std::uint32_t
    {
        None        = 0,
        Vertex      = 1 << 0,
        Index       = 1 << 1,
        Uniform     = 1 << 2,
        Storage     = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5,
    };
    using BufferUsageFlags = Flags<BufferUsageFlagBits>;
}

// Texture Interface
// ================================
namespace RHI
{
    [[nodiscard]] inline uint32_t getMipLevels(const Extent2D& extent) noexcept
    {
        return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    }

    enum class TextureType
    {
        e2D,
        e3D,
    };

    [[nodiscard]] inline TextureType getTextureType(const Extent3D extent) noexcept
    {
        return extent.depth == 1 ? TextureType::e2D : TextureType::e3D;
    }

    #ifdef nbl_VulkanRHI
    [[nodiscard]] constexpr vk::ImageType to_vk(const TextureType type) noexcept
    {
        using enum TextureType;
        switch (type)
        {
            case e2D: return vk::ImageType::e2D;
            case e3D: return vk::ImageType::e3D;
        }
        exitWithError("Unhandled TextureType");
    }
    #endif

    #ifdef nbl_MetalRHI
    [[nodiscard]] constexpr MTL::TextureType to_mtl(const TextureType type) noexcept
    {
        using enum TextureType;
        switch (type)
        {
            case e2D: return MTL::TextureType2D;
            case e3D: return MTL::TextureType3D;
        }
        exitWithError("Unhandled TextureType");
    }
    #endif

    enum class TextureUsage
    {
        Undefined,
        ColorAttachment,
        DepthAttachment,
        Clear,
        General,
        ShaderReadOnly,
        StorageImage,
        TransferSrc,
        TransferDst,
        PresentSrc,
    };
    
    enum class TextureUsageFlagBits : std::uint32_t
    {
        None                   = 0,
        TransferSrc            = 1 << 0,
        TransferDst            = 1 << 1,
        Sampled                = 1 << 2,
        Storage                = 1 << 3,
        ColorAttachment        = 1 << 4,
        DepthStencilAttachment = 1 << 5,
    };
    nbl_DECL_FLAG_TYPE(TextureUsageFlags, TextureUsageFlagBits);

    #ifdef nbl_MetalRHI
    [[nodiscard]] constexpr MTL::TextureUsage to_mtl(const TextureUsageFlags flags) noexcept
    {
        MTL::TextureUsage usage = MTL::TextureUsageUnknown;

        if (flags.contains(TextureUsageFlagBits::Sampled))
        {
            usage |= MTL::TextureUsageShaderRead;
        }
        if (flags.contains(TextureUsageFlagBits::Storage))
        {
            usage |= MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;
        }
        if (flags.contains(TextureUsageFlagBits::ColorAttachment) || flags.contains(TextureUsageFlagBits::DepthStencilAttachment))
        {
            usage |= MTL::TextureUsageRenderTarget;
        }

        return usage;
    }
    #endif

    enum class TextureAspectFlagBits : std::uint32_t
    {
        None    = 0,
        Color   = 1 << 0,
        Depth   = 1 << 1,
        Stencil = 1 << 2,
    };
    nbl_DECL_FLAG_TYPE(TextureAspectFlags, TextureAspectFlagBits);

    // Get the TextureAspectFlags for a Format
    [[nodiscard]] constexpr TextureAspectFlags getAspectFlags(const Format format) noexcept
    {
        TextureAspectFlags result = TextureAspectFlagBits::None;
        if (isColor(format))
        {
            result |= TextureAspectFlagBits::Color;
        }
        if (hasDepthComponent(format))
        {
            result |= TextureAspectFlagBits::Depth;
        }
        return result;
    }

    struct TextureProperties
    {
        Extent3D            extent;
        Format              format;
        TextureAspectFlags  aspectFlags;
        uint32_t            levelCount;
        uint32_t            sampleCount;
        TextureType         type;
    };
    
    // Vulkan related conversions and types
    // TODO: Move these to VulkanRHI somehow
    // ====================================
    #pragma region
    #ifdef nbl_VulkanRHI_
    struct VulkanImageState
    {
        vk::ImageLayout         layout     = vk::ImageLayout::eUndefined;
        vk::AccessFlags2        accessMask = vk::AccessFlagBits2::eNone;
        vk::PipelineStageFlags2 stageMask  = vk::PipelineStageFlagBits2::eNone;
    };
    
    // Convert from an RHI TextureUsage to VulkanImageState
    [[nodiscard]] constexpr VulkanImageState getVulkanImageState(const TextureUsage usage) noexcept
    {
        using L = vk::ImageLayout;
        using A = vk::AccessFlagBits2;
        using S = vk::PipelineStageFlagBits2;

        using enum TextureUsage;
        switch (usage)
        {
            case Undefined: {
                return { L::eUndefined, A::eNone, S::eNone };
            }
            case ColorAttachment: {
                return {
                    .layout     = L::eColorAttachmentOptimal,
                    .accessMask = A::eColorAttachmentRead | A::eColorAttachmentWrite,
                    .stageMask  = S::eColorAttachmentOutput,
                };
            }
            case DepthAttachment: {
                return {
                    .layout     = L::eDepthStencilAttachmentOptimal,
                    .accessMask = A::eDepthStencilAttachmentRead | A::eDepthStencilAttachmentWrite,
                    .stageMask  = S::eColorAttachmentOutput | S::eEarlyFragmentTests | S::eLateFragmentTests,
                };
            }
            case Clear: {
                return {
                    .layout     = L::eTransferDstOptimal,
                    .accessMask = A::eTransferWrite,
                    .stageMask = S::eClear,
                };
            }
            case General: {
                return {
                    .layout     = L::eGeneral,
                    .accessMask = A::eMemoryRead | A::eMemoryWrite,
                    .stageMask = S::eAllCommands,
                };
            }
            case ShaderReadOnly: {
                return {
                    .layout     = L::eShaderReadOnlyOptimal,
                    .accessMask = A::eShaderRead | A::eShaderStorageRead | A::eShaderSampledRead,
                    .stageMask  = S::eAllCommands,
                };
            }
            case StorageImage: {
                return {
                    .layout     = L::eGeneral,
                    .accessMask = A::eShaderRead | A::eShaderWrite | A::eShaderStorageRead | A::eShaderStorageWrite,
                    .stageMask = S::eAllCommands,
                };
            }
            case TransferSrc: {
                return {
                    .layout     = L::eTransferSrcOptimal,
                    .accessMask = A::eTransferRead,
                    .stageMask  = S::eAllTransfer,
                };
            }
            case TransferDst: {
                return {
                    .layout     = L::eTransferDstOptimal,
                    .accessMask = A::eTransferWrite,
                    .stageMask  = S::eAllTransfer,
                };
            }
            case PresentSrc: {
                return {
                    .layout     = L::ePresentSrcKHR,
                    .accessMask = A::eNone,
                    .stageMask  = S::eBottomOfPipe
                };
            }
        }
        exitWithError("Unknown TextureUsage!");
    }

    /**
     * Get the common image usage flags for a certain usage.
     * @note All Images receive TransferSrc and TransferDst flags by default.
     */
    [[nodiscard]] constexpr vk::ImageUsageFlags getVulkanImageUsageFlags(const TextureUsage usage) noexcept
    {
        using enum TextureUsage;
        using enum vk::ImageUsageFlagBits;

        vk::ImageUsageFlags usageFlags = eTransferSrc | eTransferDst;
        switch (usage)
        {
            case ColorAttachment: {
                usageFlags |= eColorAttachment;
                break;
            }
            case DepthAttachment: {
                usageFlags |= eDepthStencilAttachment;
                break;
            }
            case ShaderReadOnly: {
                usageFlags |= eSampled | eStorage;
                break;
            }
            case StorageImage: {
                usageFlags |= eStorage;
                break;
            }
            default: {
                usageFlags = {};
            }
        }

        return usageFlags;
    }

    // Get the aspect flags for a specific format.
    [[nodiscard]] constexpr vk::ImageAspectFlags getImageAspectFlags(const Format format) noexcept
    {
        vk::ImageAspectFlags aspectFlags = {};

        if (isColor(format))
        {
            aspectFlags = vk::ImageAspectFlagBits::eColor;
        }
        if (hasDepthComponent(format))
        {
            aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        }

        return aspectFlags;
    }
    #endif
    #pragma endregion

    struct TextureCreateInfo
    {
        Extent3D            extent;
        Format              format;
        TextureUsageFlags   usageFlags;
        std::string         label;

        [[nodiscard]] TextureType getTextureType() const noexcept
        {
            return extent.depth == 1 ? TextureType::e2D : TextureType::e3D;
        }
    };

    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        virtual void updateTrackedState(TextureUsage dstUsage) noexcept = 0;

        [[nodiscard]] virtual TextureUsage getCurrentState() const noexcept = 0;

        [[nodiscard]] virtual const TextureProperties& getProperties() const noexcept = 0;
    };
}

// Swapchain Interface
// ================================
namespace RHI
{
    // Define the preferred Swapchain parameters for creation.
    struct SwapchainPreferences
    {
        Format      format      = Format::BGRA8Unorm;
        PresentMode presentMode = platform::isApple ? PresentMode::Immediate : PresentMode::Mailbox;
    };

    class ISwapchain
    {
    public:
        virtual ~ISwapchain() = default;
    };
}

// CommandList Interface
// ================================
namespace RHI
{
    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        /**
         * Begin the recording of a CommandList
         */
        virtual void begin() noexcept = 0;

        /**
         * End the recording of a CommandList
         */
        virtual void end() noexcept = 0;

        /**
         * Opens a CommandList debug label region
         * @param label region name
         */
        virtual void beginLabel(std::string_view label) const noexcept = 0;
        virtual void beginLabel(std::string_view label, const std::array<float, 3>& color) const noexcept = 0;

        /**
         * Ends the last opened CommandList debug label region
         */
        virtual void endLabel() const noexcept = 0;

        // ================================
        // Transfer Operations
        // ================================

        /**
         * Copy data between buffer regions
         * @param copyInfo src, dst buffers and region descriptions
         */
        virtual void copyBuffer(const RHI::CopyBufferInfo& copyInfo) const noexcept = 0;

        /**
         * Copy data from a Buffer to an Image
         * @param copyInfo Buffer and Image handles
         */
        virtual void copyBufferToImage(const RHI::BufferImageCopyInfo& copyInfo) const noexcept = 0;

        // ================================
        // Rendering Operations
        // ================================

        /**
         * Set the scissor rectangle
         * @param scissor
         */
        virtual void setScissor(const RHI::Rect2D& scissor) const noexcept = 0;

        /**
         * Set the viewport
         * @param viewport
         */
        virtual void setViewport(const RHI::Viewport& viewport) const noexcept = 0;

        // TODO: Add parameter struct
        virtual void beginRendering() const noexcept = 0;

        virtual void endRendering() const noexcept = 0;

        virtual void bindPipeline(RHI::Pipeline* pPipeline) const noexcept = 0;

        // virtual void bindDescriptorSets(RHI::Pipeline* pPipeline, const std::vector<RHI::Descriptor*>& descriptorSets) noexcept = 0;

        // virtual void pushConstants(RHI::Pipeline* pPipeline, void* pData) const noexcept = 0;

        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const noexcept = 0;

        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const noexcept = 0;

        virtual void bindVertexBuffers(uint32_t firstBinding, const std::vector<RHI::Buffer*>& buffers, const std::vector<DeviceSize>& offsets) const noexcept = 0;

        virtual void bindIndexBuffer(RHI::Buffer* pBuffer, RHI::DeviceSize offset, RHI::IndexType indexType) const noexcept = 0;

        // ================================
        // Synchronization
        // ================================

        virtual void insertBarrier(const RHI::DependencyInfo& dependencyInfo) const noexcept = 0;
    };
}

// DynamicRHI Interface
// ================================
namespace RHI
{
    class DynamicRHI
    {
    public:
        virtual ~DynamicRHI() = default;

        [[nodiscard]] virtual FrameData beginFrame() noexcept = 0;
        virtual void endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const = 0;

        [[nodiscard]] virtual SPtr<ITexture> createTexture(const TextureCreateInfo& createInfo) noexcept = 0;
    };
}
