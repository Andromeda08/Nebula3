#include "VulkanRHI.hpp"

#include <limits>
#include <spdlog/spdlog.h>

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

        mDevice = Device::create({ mFeatureLevel, mInstance->getHandle() });
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

        mFrameSync = std::make_unique<FrameSync>(mDevice);

        const auto& swapchainProperties = mSwapchain->getProperties();
        spdlog::debug("[RHI] Created VulkanRHI\n\t- Device: {}\n\t- Feature Level: {}\n\t- Debug Features: {}\n\t- Swapchain Details: [images={}, format={}, colorSpace={}, presentMode={}]",
            mDevice->getDeviceName(), "TODO FEATURE LEVEL", config.rhi.debugFeatures ? "Yes" : "No",
            mSwapchain->getImageCount(), vk::to_string(swapchainProperties.format), vk::to_string(swapchainProperties.colorSpace), vk::to_string(swapchainProperties.presentMode));
    }

    FrameData VulkanRHI::beginFrame() const
    {
        const vk::Device device = mDevice->getHandle();

        const uint64_t currentFrame = mFrameSync->currentFrame;
        const vk::Fence fence = mFrameSync->frameInFlight[currentFrame];

        const vk::Result result = device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
        exitOnAssert(result == vk::Result::eSuccess, "vk::Device::waitForFences failed: {}", vk::to_string(result));

        device.resetFences(fence);

        const auto acquireInfo = vk::AcquireNextImageInfoKHR()
            .setSwapchain(mSwapchain->getHandle())
            .setTimeout(std::numeric_limits<uint64_t>::max())
            .setSemaphore(mFrameSync->imageReady[currentFrame])
            .setDeviceMask(1);
        const auto acquiredIndex = device.acquireNextImage2KHR(acquireInfo).value;

        return {
            .waitFence                  = mFrameSync->frameInFlight[currentFrame],
            .imageReadySemaphore        = mFrameSync->imageReady[currentFrame],
            .renderingFinishedSemaphore = mFrameSync->renderingFinished[currentFrame],
            .currentFrame               = currentFrame,
            .acquiredIndex              = acquiredIndex,
        };
    }

    void VulkanRHI::endFrame_submitAndPresent(const PresentSubmitInfo& presentSubmitInfo) const
    {
        const auto& frameData = presentSubmitInfo.frameData;

        const auto info = vk::CommandBufferSubmitInfo()
                .setCommandBuffer(presentSubmitInfo.pCommandList->getHandle());
        std::vector commandBufferSubmitInfos { info };

        const auto waitSemaphoreInfo = vk::SemaphoreSubmitInfo()
                .setSemaphore(frameData.imageReadySemaphore)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        std::vector waitSemaphoreInfos { waitSemaphoreInfo };

        const auto signalSemaphoreInfo = vk::SemaphoreSubmitInfo()
                .setSemaphore(frameData.renderingFinishedSemaphore)
                .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
        std::vector signalSemaphoreInfos { signalSemaphoreInfo };

        const auto vkSubmitInfo = vk::SubmitInfo2()
            .setCommandBufferInfos(commandBufferSubmitInfos)
            .setCommandBufferInfoCount(commandBufferSubmitInfos.size())
            .setWaitSemaphoreInfos(waitSemaphoreInfos)
            .setWaitSemaphoreInfoCount(waitSemaphoreInfos.size())
            .setSignalSemaphoreInfos(signalSemaphoreInfos)
            .setSignalSemaphoreInfoCount(signalSemaphoreInfos.size());

        mDevice->getGraphicsQueue().queue.submit2(vkSubmitInfo, frameData.waitFence);

        const auto swapchain = mSwapchain->getHandle();
        const auto presentInfo = vk::PresentInfoKHR()
            .setPWaitSemaphores(&frameData.renderingFinishedSemaphore)
            .setWaitSemaphoreCount(1)
            .setPSwapchains(&swapchain)
            .setSwapchainCount(1)
            .setImageIndices(frameData.acquiredIndex)
            .setPResults(nullptr);

        const auto result = mDevice->getGraphicsQueue().queue.presentKHR(presentInfo);
        exitOnAssert(result == vk::Result::eSuccess, "Failed to present image.");

        mFrameSync->advanceCurrentFrame();
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
    
    SPtr<Descriptor> VulkanRHI::createDescriptor(const RHIDescriptorCreateInfo& createInfo) const
    {
        auto descriptorInfo = DescriptorCreateInfo(createInfo);
        descriptorInfo.device = mDevice;
        return Descriptor::create(descriptorInfo);
    }

    SPtr<Texture> VulkanRHI::createTexture(const RHITextureCreateInfo& createInfo) const
    {
        auto textureInfo = TextureCreateInfo(createInfo);
        textureInfo.device = mDevice;
        return Texture::create(textureInfo);
    }

    UPtr<GraphicsPipeline> VulkanRHI::createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const
    {
        createInfo.setDevice(mDevice);
        return GraphicsPipeline::create(createInfo);
    }

    UPtr<ComputePipeline> VulkanRHI::createComputePipeline(ComputePipelineCreateInfo& createInfo) const
    {
        createInfo.setDevice(mDevice);
        return ComputePipeline::create(createInfo);
    }

    UPtr<RaytracingPipeline> VulkanRHI::createRaytracingPipeline(RaytracingPipelineCreateInfo createInfo) const
    {
        if (mFeatureLevel != RHIFeatureLevel::Complete)
        {
            exitWithError("[RHI] Error: Raytracing Pipeline is not supported at the current feature level!");
        }
        createInfo.setDevice(mDevice);
        return RaytracingPipeline::create(createInfo);
    }

    UPtr<RenderPass> VulkanRHI::createRenderPass(const RenderPassCreateInfo& createInfo) const
    {
        auto privateCreateInfo = RenderPassCreateInfo(createInfo);
        if (createInfo.renderArea.extent.width == 0 && createInfo.renderArea.extent.height == 0)
        {
            privateCreateInfo.renderArea = mSwapchain->getProperties().area;
        }
        return RenderPass::create(privateCreateInfo);
    }

    void VulkanRHI::immediate_uploadToBuffer(const Buffer* pDst, const void* pData, const uint64_t size, const uint64_t srcOffset, const uint64_t dstOffset) const noexcept
    {
        const auto staging = createBuffer({
            .size  = size,
            .type  = BufferType::Staging,
            .label = std::format("StagingBuffer-DstId-{}", pDst->getId()),
        });
        staging->setData(pData, size, srcOffset);

        mGraphicsQueue->immediate([&](const CommandList* pCommandList) -> void {
            const auto region = vk::BufferCopy2()
                .setSrcOffset(srcOffset)
                .setDstOffset(dstOffset)
                .setSize(size);
            const auto copy = vk::CopyBufferInfo2()
                .setSrcBuffer(staging->getHandle())
                .setDstBuffer(pDst->getHandle())
                .setRegions(region);
            pCommandList->getHandle().copyBuffer2(copy);
        });
    }
}
