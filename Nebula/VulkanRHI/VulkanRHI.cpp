#include "VulkanRHI.hpp"

#include <print>

#include "Core/ToString.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace RHI
{
    VulkanRHI::VulkanRHI(const VulkanRHICreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    {
        const auto& config = Configuration::getConfig();
        mFeatureLevel = config.rhi.featureLevel;

        const vk::detail::DynamicLoader dynamicLoader;
        const auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        mInstance = Instance::create({ mWindow.get() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance->getHandle());

        if (config.rhi.debugFeatures)
        {
            mDebugContext = DebugContext::create({ mInstance });
        }

        mDevice = Device::create({ mInstance->getHandle() });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice->getHandle());

        mSwapchain = Swapchain::create({
            .window   = mWindow,
            .instance = mInstance,
            .device   = mDevice,
        });

        mGraphicsQueue = CommandQueue::create({
            .queue  = mDevice->getGraphicsQueue(),
            .device = mDevice,
        });

        mFrameSync = std::make_unique<FrameSync>(mDevice.get());

        const auto& swapchainProperties = mSwapchain->getProperties();
        std::println("[RHI] Created VulkanRHI\n\t- Device: {}\n\t- Feature Level: {}\n\t- Debug Features: {}\n\t- Swapchain Details: [images={}, format={}, colorSpace={}, presentMode={}]",
            mDevice->getDeviceName(), toString(mFeatureLevel), toString(config.rhi.debugFeatures),
            mSwapchain->getImageCount(), vk::to_string(swapchainProperties.format), vk::to_string(swapchainProperties.colorSpace), vk::to_string(swapchainProperties.presentMode));
    }

    SPtr<Buffer> VulkanRHI::createBuffer(const RHIBufferCreateInfo& createInfo) const
    {
        auto bufferInfo = BufferCreateInfo(createInfo);
        bufferInfo.device = mDevice;
        return Buffer::create(bufferInfo);
    }

    SPtr<Image> VulkanRHI::createImage(const RHIImageCreateInfo& createInfo) const
    {
        auto imageInfo = ImageCreateInfo(createInfo);
        imageInfo.device = mDevice;
        return Image::create(imageInfo);
    }

    UPtr<GraphicsPipeline> VulkanRHI::createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const
    {
        createInfo.device = mDevice;
        return GraphicsPipeline::create(createInfo);
    }

    UPtr<ComputePipeline> VulkanRHI::createComputePipeline(ComputePipelineCreateInfo& createInfo) const
    {
        createInfo.device = mDevice;
        return ComputePipeline::create(createInfo);
    }

    UPtr<RenderPass> VulkanRHI::createRenderPass(const RenderPassCreateInfo& createInfo) const
    {
        return RenderPass::create(createInfo);
    }
}
