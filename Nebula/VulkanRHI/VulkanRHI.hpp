#pragma once

#include <vulkan/vulkan.hpp>

#include "Allocation.hpp"
#include "Buffer.hpp"
#include "Commands.hpp"
#include "DebugContext.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "IWindow.hpp"
#include "Raytracing.hpp"
#include "Rendering.hpp"
#include "Swapchain.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"
#include "Render/Pipeline.hpp"

namespace RHI
{
    struct VulkanRHICreateInfo
    {
        SPtr<IWindow> pWindow = nullptr;
    };

    struct PendingDelete
    {
        SPtr<Resource>  buffer;
        uint64_t        frameToRelease;
    };

    class VulkanRHI
    {
    public:
        nbl_DISABLE_COPY(VulkanRHI);
        nbl_CTOR_SHARED(VulkanRHI);

        [[nodiscard]] FrameData beginFrame() const;

        void endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const;

        // Check if the specified frame has been completed using the corresponding Fence.
        [[nodiscard]] bool isFrameComplete(uint64_t frame) const;

        [[nodiscard]] SPtr<Instance> getInstance()      const { return mInstance; }
        [[nodiscard]] SPtr<Device>   getDevice()        const { return mDevice; }
        [[nodiscard]] CommandQueue*  getGraphicsQueue() const { return mGraphicsQueue.get(); }
        [[nodiscard]] Swapchain*     getSwapchain()     const { return mSwapchain.get(); }

        [[nodiscard]] SPtr<Buffer>     createBuffer    (const RHIBufferCreateInfo&     createInfo) const;
        [[nodiscard]] SPtr<Image>      createImage     (const RHIImageCreateInfo&      createInfo) const;
        [[nodiscard]] SPtr<Descriptor> createDescriptor(const RHIDescriptorCreateInfo& createInfo) const;

        UPtr<GraphicsPipeline2>  createGraphicsPipeline2(GraphicsPS ps, const PipelineCommon& common) const;

        UPtr<GraphicsPipeline>   createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
        UPtr<ComputePipeline>    createComputePipeline(ComputePipelineCreateInfo& createInfo) const;
        UPtr<RaytracingPipeline> createRaytracingPipeline(RaytracingPipelineCreateInfo createInfo) const;

        UPtr<RenderPass> createRenderPass(const RenderPassCreateInfo& createInfo) const;

        void immediate_uploadToBuffer(const Buffer* pDst, const void* pData, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const noexcept;

        [[nodiscard]] const RHIFeatures& getFeatures() const
        {
             return gFeatures;
        }

    private:
        SPtr<IWindow>       mWindow;

        SPtr<Instance>      mInstance;
        UPtr<DebugContext>  mDebugContext;
        SPtr<Device>        mDevice;
        UPtr<Swapchain>     mSwapchain;
        UPtr<CommandQueue>  mGraphicsQueue;
        UPtr<FrameSync>     mFrameSync;

        std::vector<PendingDelete> mDeletionQueue;
    };
}
