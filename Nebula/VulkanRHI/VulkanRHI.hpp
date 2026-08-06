#pragma once

#include <vulkan/vulkan.hpp>

#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>
#include <nvsdk_ngx_helpers_dlssd_vk.h>

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
#include "Integration/DLSS.hpp"
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

        [[nodiscard]] SPtr<Buffer>     createBuffer    (const RHIBufferCreateInfo&     createInfo) const;
        [[nodiscard]] SPtr<Image>      createImage     (const RHIImageCreateInfo&      createInfo) const;
        [[nodiscard]] SPtr<Descriptor> createDescriptor(const RHIDescriptorCreateInfo& createInfo) const;

        [[nodiscard]] UPtr<ComputePipeline2> createComputePipeline2(const PipelineCommon& common) const;

        [[nodiscard]] UPtr<GraphicsPipeline2> createGraphicsPipeline2(GraphicsPS ps, const PipelineCommon& common) const;

        [[nodiscard]] UPtr<RayTracingPipeline2> createRayTracingPipeline2(RayTracingPS ps, const PipelineCommon& common) const;

        void immediate_uploadToBuffer(const Buffer* pDst, const void* pData, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const noexcept;

        [[nodiscard]] SPtr<Instance> getInstance()      const { return mInstance; }
        [[nodiscard]] SPtr<Device>   getDevice()        const { return mDevice; }
        [[nodiscard]] CommandQueue*  getGraphicsQueue() const { return mGraphicsQueue.get(); }
        [[nodiscard]] Swapchain*     getSwapchain()     const { return mSwapchain.get(); }

        [[nodiscard]] static const RHIFeatures& getFeatures()
        {
             return gFeatures;
        }

        [[deprecated("Use createGraphicsPipeline2")]]
        [[nodiscard]] UPtr<GraphicsPipeline>   createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;

        [[deprecated("Use createComputePipeline2")]]
        [[nodiscard]] UPtr<ComputePipeline>    createComputePipeline(ComputePipelineCreateInfo& createInfo) const;

        [[deprecated("Use createRaytracingPipeline2")]]
        [[nodiscard]] UPtr<RaytracingPipeline> createRaytracingPipeline(RaytracingPipelineCreateInfo createInfo) const;

        Integration::DLSS* getDLSS() const;

    private:
        SPtr<IWindow>               mWindow;

        SPtr<Instance>              mInstance;
        UPtr<DebugContext>          mDebugContext;
        SPtr<Device>                mDevice;
        UPtr<Swapchain>             mSwapchain;
        UPtr<CommandQueue>          mGraphicsQueue;
        UPtr<FrameSync>             mFrameSync;

        std::vector<PendingDelete>  mDeletionQueue;

        UPtr<Integration::DLSS>     mDLSS;
    };
}
