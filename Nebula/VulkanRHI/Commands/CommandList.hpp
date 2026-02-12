#pragma once

#include <vulkan/vulkan.hpp>

#include "Core/Macro.hpp"
#include "RHI/RHI.hpp"
#include "VulkanRHI/VulkanCore.hpp"

namespace RHI
{
    class Buffer;
    class Image;

    struct CommandListCreateInfo
    {
        vk::CommandBuffer  commandBuffer;
        bool               singleTimeSubmit = false;
    };

    class CommandList : ICommandList
    {
    public:
        nbl_DISABLE_COPY(CommandList);

        // ❗(Lifetime) mCommandBuffer is allocated by VulkanCommandPool::allocate();
        nbl_CTOR(CommandList);

        // ❗(Lifetime) mCommandBuffer is freed by VulkanCommandPool::free();
        ~CommandList() override = default;

        void begin() noexcept override;
        void end() noexcept override;

        void beginLabel(std::string_view label) const noexcept override;
        void beginLabel(std::string_view label, const std::array<float, 3>& color) const noexcept override;
        void endLabel() const noexcept override;

        void copyBufferToImage(const BufferImageCopyInfo& copyInfo) const noexcept override;

        [[nodiscard]] vk::CommandBuffer getHandle() const noexcept { return mCommandBuffer; }

        void copyBuffer(const CopyBufferInfo& copyInfo) const noexcept override;

        void setScissor(const Rect2D& scissor) const noexcept override;

        void setViewport(const Viewport& viewport) const noexcept override;

        void beginRendering() const noexcept override;

        void endRendering() const noexcept override;

        void bindPipeline(Pipeline* pPipeline) const noexcept override;

        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const noexcept override;

        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const noexcept override;

        void bindVertexBuffers(uint32_t firstBinding, const std::vector<Buffer*>& buffers, const std::vector<DeviceSize>& offsets) const noexcept override;

        void bindIndexBuffer(Buffer* pBuffer, DeviceSize offset, IndexType indexType) const noexcept override;

        void insertBarrier(const DependencyInfo& dependencyInfo) const noexcept override;

    private:
        vk::CommandBuffer   mCommandBuffer;

        bool                mIsRecording = false;
        const bool          mSingleTime  = false;
        const bool          mDebug       = false;
    };
}
