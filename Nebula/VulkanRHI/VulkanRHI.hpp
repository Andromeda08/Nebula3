#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.hpp"
#include "DebugContext.hpp"
#include "Device.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "IWindow.hpp"
#include "Rendering.hpp"
#include "Swapchain.hpp"
#include "VulkanCore.hpp"
#include "Commands/CommandQueue.hpp"
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
        nbl_CTOR(VulkanRHI);

        // FrameData beginFrame();
        // void      submit(const SubmitInfo& submitInfo);
        // void      present(const FrameData& frameData);

        SPtr<Buffer> createBuffer(const RHIBufferCreateInfo& createInfo) const;
        SPtr<Image>  createImage(const RHIImageCreateInfo& createInfo) const;

        UPtr<GraphicsPipeline> createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
        UPtr<ComputePipeline>  createComputePipeline(ComputePipelineCreateInfo& createInfo) const;

        UPtr<RenderPass> createRenderPass(const RenderPassCreateInfo& createInfo) const;

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