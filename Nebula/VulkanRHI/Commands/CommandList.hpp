#pragma once

#include <vulkan/vulkan.hpp>

#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    class Buffer;
    class Image;

    struct BufferRegion
    {
        uint64_t srcOffset;
        uint64_t dstOffset;
        uint64_t size;
    };

    struct BufferCopyInfo
    {
        Buffer*                     src     = nullptr;
        Buffer*                     dst     = nullptr;
        std::vector<BufferRegion>   regions = {};
    };

    struct BufferImageCopyInfo
    {
        Buffer*  pSrcBuffer;
        Image*   pDstImage;
        uint64_t bufferOffset = 0;
    };

    struct CommandListCreateInfo
    {
        vk::CommandBuffer  commandBuffer;
        bool               singleTimeSubmit = false;
    };

    class CommandList
    {
    public:
        nbl_DISABLE_COPY(CommandList);

        // ❗(Lifetime) mCommandBuffer is allocated by VulkanCommandPool::allocate();
        nbl_CTOR(CommandList);

        // ❗(Lifetime) mCommandBuffer is freed by VulkanCommandPool::free();
        ~CommandList() = default;

        void begin();
        void end();

        void beginLabel(const std::array<float, 3>& color, const std::string& name) const;
        void beginLabel(const std::string& name) const;
        void endLabel() const;

        void copyBufferToImage(const BufferImageCopyInfo& copyInfo) const;

        void copyBuffer(const BufferCopyInfo& bufferCopyInfo) const;

        [[nodiscard]] vk::CommandBuffer getHandle() const noexcept { return mCommandBuffer; }

    private:
        vk::CommandBuffer   mCommandBuffer;

        bool                mIsRecording = false;
        const bool          mSingleTime  = false;
        const bool          mDebug       = false;
    };
}