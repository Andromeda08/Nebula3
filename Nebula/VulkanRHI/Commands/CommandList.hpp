#pragma once

#include <vulkan/vulkan.hpp>

#include "RHI/DynamicRHI.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/Device.hpp"

namespace RHI
{
    class Buffer;
    class Descriptor;
    class Image;
    class PipelineBase;
    class Swapchain;
}

namespace RHI
{
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
        Buffer*  pSrcBuffer   = nullptr;
        Image*   pDstImage    = nullptr;
        uint64_t bufferOffset = 0;
    };

    struct CommandListCreateInfo
    {
        vk::CommandBuffer  commandBuffer;
        bool               singleTimeSubmit = false;
    };

    class CommandList final : public ICommandList
    {
    public:
        nbl_DISABLE_COPY(CommandList);

        // ❗(Lifetime) mCommandBuffer is allocated by VulkanCommandPool::allocate();
        nbl_CTOR(CommandList);

        // ❗(Lifetime) mCommandBuffer is freed by VulkanCommandPool::free();
        ~CommandList() override = default;

        // Begin recording the CommandList
        void begin() override;

        // End the recording of the CommandList
        void end() override;

        void setViewportScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor) const;

        // Binds a pipeline for use.
        void bindPipeline(PipelineBase* pPipeline);

        // Bind a descriptor set at the specified index.
        void bindDescriptorSet(const vk::DescriptorSet& descriptorSet, uint32_t setIndex = 0) const;

        void bindDescriptorSets(const std::vector<vk::DescriptorSet>& descriptorSets, uint32_t firstSet = 0) const;

        // Push constants, configuration based on the currently bound pipeline.
        void pushConstants(const void* pData) const;

        #pragma region "Draw, Dispatch, TraceRays"

        // Draw
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

        // Draw Indexed
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const;

        // Draw Indexed Indirect (assumes stride of VkDrawIndexedIndirectCommand by default)
        void drawIndexedIndirect(const Buffer* pBuffer, uint64_t offset, uint32_t drawCount, uint32_t stride = sizeof(vk::DrawIndexedIndirectCommand)) const;

        // Dispatch a Compute Pipeline with the specified group sizes.
        void dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) const;

        // Indirect dispatch of a Compute Pipeline with group sizes sourced from a Buffer.
        void dispatchIndirect(const Buffer* pBuffer, uint64_t offset) const;

        /**
         * Dispatch a Ray Tracing Pipeline with sizes and maximum ray recursion depth.
         * TODO: Authored on Apple, test on NVIDIA
         */
        void traceRays(uint32_t x, uint32_t y, uint32_t rayDepth = 1) const;

        void drawMeshTasks(uint32_t x, uint32_t y, uint32_t z) const;

        #pragma endregion

        #pragma region "Debug Markers"

        [[deprecated("use the function with the new parameter order")]]
        void beginLabel(const std::array<float, 3>& color, const std::string& name) const;

        /**
         * Begin a debug marker region with the specified label and color.
         */
        void beginLabel(const std::string& label, const std::array<float, 3>& color) const;

        /**
         * Begin a debug marker region with the specified label and randomly generated color.
         */
        void beginLabel(const std::string& name) const;

        /**
         * End the current debug marker region.
         */
        void endLabel() const;

        #pragma endregion

        /**
         * Blit from the src image to the swapchain image specified by the ACQUIRED image index.
         * Note: this function will insert the required pre-blit barriers.
         * @param pSrcImage
         * @param pSwapchain
         * @param acquiredIndex
         */
        void blitToSwapchain(Image* pSrcImage, Swapchain* pSwapchain, uint32_t acquiredIndex) const;

        void copyBufferToImage(const BufferImageCopyInfo& copyInfo) const;

        void copyBuffer(const BufferCopyInfo& bufferCopyInfo) const;

        [[nodiscard]] vk::CommandBuffer getHandle() const noexcept { return mCommandBuffer; }

    private:
        vk::CommandBuffer   mCommandBuffer;

        PipelineBase*       mBoundPipeline = nullptr;

        bool                mIsRecording = false;
        const bool          mSingleTime  = false;
        const bool          mDebug       = false;
    };
}
