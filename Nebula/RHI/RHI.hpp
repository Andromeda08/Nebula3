#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "VulkanRHI/Buffer.hpp"

// Conversion functions are only available when the backend specific macro is defined.
// Vulkan
// - As methods on objects  : .vk( )
// - As standalone functions:  vk(T)
#define nbl_VulkanRHI
// Metal
// - As methods on objects  : .mtl( )
// - As standalone functions:  mtl(T)
#define nbl_MetalRHI

// Vulkan Specifics
// ================================
#ifdef nbl_VulkanRHI
    #include <vulkan/vulkan.hpp>
#endif

// Metal Specifics
// ================================
#ifdef nbl_MetalRHI
    #include <metal/metal.hpp>
#endif

// Forward Declarations
// ================================
namespace RHI
{
    enum class ImageUsage;
    // Resources
    class Buffer;
    class Descriptor;
    class Image;
    class Pipeline;
    class RenderPass;

    // Commands
    class Barrier;
    class CommandList;
    class CommandPool;
    class CommandQueue;
}

namespace RHI2
{
    using DeviceSize = std::uint64_t;

    /**
     * Basic Buffer to Image copy operation
     */
    struct BufferImageCopyInfo
    {
        RHI::Buffer* pSrcBuffer;
        RHI::Image*  pDstImage;
    };

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
        RHI::Buffer*              srcBuffer = nullptr;
        RHI::Buffer*              dstBuffer = nullptr;
        std::vector<BufferRegion> regions   = {};

        CopyBufferInfo& setSrcBuffer(RHI::Buffer* pBuffer) noexcept
        {
            srcBuffer = pBuffer;
            return *this;
        }

        CopyBufferInfo& setDstBuffer(RHI::Buffer* pBuffer) noexcept
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
        Uint32,
    };

    [[nodiscard]] constexpr vk::IndexType to_vk(const IndexType indexType) noexcept
    {
        if (indexType == IndexType::Uint32)
        {
            return vk::IndexType::eUint32;
        }
        std::unreachable();
    }

    // ================================
    // Resource States, Synchronization
    // ================================
    enum class ImageUsage
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

    struct ImageMemoryBarrier
    {
        RHI::Image*      pImage;
        RHI::ImageUsage  dstUsage;
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

    // CommandList Interface
    // ================================
    class CommandList
    {
    public:
        virtual ~CommandList() = default;

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
        virtual void copyBuffer(const CopyBufferInfo& copyInfo) const noexcept = 0;

        /**
         * Copy data from a Buffer to an Image
         * @param copyInfo Buffer and Image handles
         */
        virtual void copyBufferToImage(const BufferImageCopyInfo& copyInfo) const noexcept = 0;

        // ================================
        // Rendering Operations
        // ================================

        /**
         * Set the scissor rectangle
         * @param scissor
         */
        virtual void setScissor(const Rect2D& scissor) const noexcept = 0;

        /**
         * Set the viewport
         * @param viewport
         */
        virtual void setViewport(const Viewport& viewport) const noexcept = 0;

        // TODO: Add parameter struct
        virtual void beginRendering() const noexcept = 0;

        virtual void endRendering() const noexcept = 0;

        virtual void bindPipeline(RHI::Pipeline* pPipeline) const noexcept = 0;

        // virtual void bindDescriptorSets(RHI::Pipeline* pPipeline, const std::vector<RHI::Descriptor*>& descriptorSets) noexcept = 0;

        // virtual void pushConstants(RHI::Pipeline* pPipeline, void* pData) const noexcept = 0;

        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const noexcept = 0;

        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const noexcept = 0;

        virtual void bindVertexBuffers(uint32_t firstBinding, const std::vector<RHI::Buffer*>& buffers, const std::vector<DeviceSize>& offsets) const noexcept = 0;

        virtual void bindIndexBuffer(RHI::Buffer* pBuffer, DeviceSize offset, IndexType indexType) const noexcept = 0;

        // ================================
        // Synchronization
        // ================================

        virtual void insertBarrier(const DependencyInfo& dependencyInfo) const noexcept = 0;
    };
}
