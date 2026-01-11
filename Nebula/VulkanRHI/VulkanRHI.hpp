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
#include "Image3D.hpp"
#include "Instance.hpp"
#include "IWindow.hpp"
#include "Raytracing.hpp"
#include "Rendering.hpp"
#include "Swapchain.hpp"
#include "VulkanCore.hpp"
#include "Core/Configuration.hpp"
#include "Core/Macro.hpp"

namespace RHI
{
    struct VulkanRHICreateInfo
    {
        SPtr<IWindow> pWindow = nullptr;
    };

    class VulkanRHI
    {
    public:
        nbl_DISABLE_COPY(VulkanRHI);
        nbl_CTOR_SHARED(VulkanRHI);

        FrameData beginFrame() const;
        void      endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const;

        CommandQueue* getGraphicsQueue() const { return mGraphicsQueue.get(); }
        Swapchain*    getSwapchain() const { return mSwapchain.get(); }

        SPtr<Buffer>     createBuffer(const RHIBufferCreateInfo& createInfo) const;
        SPtr<Image>      createImage(const RHIImageCreateInfo& createInfo) const;
        SPtr<Image3D>    createImage3D(const RHIImage3DCreateInfo& createInfo) const;
        SPtr<Descriptor> createDescriptor(const RHIDescriptorCreateInfo& createInfo) const;

        Allocation allocatedAliasedImageMemory(const std::vector<SPtr<Image>>& images) const;

        UPtr<GraphicsPipeline>   createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
        UPtr<ComputePipeline>    createComputePipeline(ComputePipelineCreateInfo& createInfo) const;
        UPtr<RaytracingPipeline> createRaytracingPipeline(RaytracingPipelineCreateInfo createInfo) const;

        UPtr<RenderPass> createRenderPass(const RenderPassCreateInfo& createInfo) const;

        SPtr<Device> getDevice() const { return mDevice; }
        SPtr<Instance> getInstance() const { return mInstance; }

        RHIFeatureLevel getFeatureLevel() const noexcept
        {
            return mFeatureLevel;
        }

    private:
        SPtr<IWindow>       mWindow;
        RHIFeatureLevel     mFeatureLevel;

        SPtr<Instance>      mInstance;
        UPtr<DebugContext>  mDebugContext;
        SPtr<Device>        mDevice;
        UPtr<Swapchain>     mSwapchain;
        UPtr<CommandQueue>  mGraphicsQueue;
        UPtr<FrameSync>     mFrameSync;
    };
}