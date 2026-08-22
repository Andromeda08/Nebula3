#include "VulkanRHI.hpp"

#include "Texture.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace sunflower::rhi
{
    VulkanRHI::VulkanRHI(const VulkanRHICreateInfo& createInfo)
    : mWindow(createInfo.pWindow)
    {
        assert(mWindow);

        const vk::detail::DynamicLoader dynamicLoader;
        const auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        mInstance = detail::Instance::create({
            .pWindow         = mWindow,
            .applicationName = createInfo.applicationName,
            .engineName      = createInfo.engineName,
        });
        VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance->getHandle());

        mSurface = detail::Surface::create({
            .pWindow  = mWindow,
            .instance = mInstance,
        });

        mDevice = Device::create({
            .instance = mInstance,
            .pSurface = mSurface.get(),
        });

        mGraphicsQueue = VulkanCommandQueue::create({
            .queue  = mDevice->getGraphicsQueue(),
            .device = mDevice,
        });

        mComputeQueue = mDevice->getComputeQueue().transform([&](const auto& q) {
            return VulkanCommandQueue::create({
                .queue  = q,
                .device = mDevice,
            });
        }).value_or(nullptr);

        mSwapchain = VulkanSwapchain::create({
            .pWindow  = mWindow,
            .pSurface = mSurface.get(),
            .device   = mDevice,
        });

        constexpr auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
        for (uint64_t i = 0; i < conf::gFramesInFlight; i++)
        {
            mImageReady[i]        = mDevice->getHandle().createSemaphore(semaphoreCreateInfo).value;
            mRenderingFinished[i] = mDevice->getHandle().createSemaphore(semaphoreCreateInfo).value;
        }
    }

    VulkanRHI::~VulkanRHI()
    {
        std::ignore = mDevice->getHandle().waitIdle();
        for (uint64_t i = 0; i < conf::gFramesInFlight; i++)
        {
            mDevice->getHandle().destroy(mImageReady[i]);
            mDevice->getHandle().destroy(mRenderingFinished[i]);
        }
    }

    FrameInfo VulkanRHI::beginFrame()
    {
        flushPendingDeletes();

        Timeline* pTimeline = mGraphicsQueue->getTimeline();

        const uint64_t nextFrameValue = pTimeline->getNextValue();
        if (nextFrameValue > conf::gFramesInFlight)
        {
            if (const bool ok = pTimeline->hostWait(nextFrameValue - conf::gFramesInFlight); !ok)
            {
                ::sunflower::exit("beginFrame timed out");
            }
        }

        const auto acquireInfo = vk::AcquireNextImageInfoKHR()
            .setSwapchain(mSwapchain->getHandle())
            .setTimeout(std::numeric_limits<uint64_t>::max())
            .setSemaphore(mImageReady[mCurrentFrameIndex])
            .setDeviceMask(1);
        const auto acquiredIndex = mDevice->getHandle().acquireNextImage2KHR(acquireInfo).value;

        return {
            .currentFrameIndex = mCurrentFrameIndex,
            .frameNumber       = mLifetimeFrameCount,
            .frameValue        = nextFrameValue,
            .acquiredIndex     = acquiredIndex,
            .pSwapchainTexture = mSwapchain->getTexture(acquiredIndex),
        };
    }

    void VulkanRHI::endFrame_submitAndPresent(const PresentFrameInfo& presentFrameInfo)
    {
        const FrameInfo& frameInfo = presentFrameInfo.frameInfo;
        const Timeline*  timeline  = mGraphicsQueue->getTimeline();

        const vk::Semaphore imageReady        = mImageReady[frameInfo.currentFrameIndex];
        const vk::Semaphore renderingFinished = mRenderingFinished[frameInfo.acquiredIndex];

        const auto submitInfo = SubmitInfo()
            .addCommandList(presentFrameInfo.pCommandList)
            .addSignal(timeline->getSync(), frameInfo.frameValue);
        mGraphicsQueue->submitWithBinarySync(submitInfo, { imageReady }, { renderingFinished });

        const auto swapchain   = mSwapchain->getHandle();
        const auto presentInfo = vk::PresentInfoKHR()
            .setWaitSemaphores(renderingFinished)
            .setSwapchains(swapchain)
            .setImageIndices(frameInfo.acquiredIndex)
            .setPResults(nullptr);

        const auto result = mDevice->getGraphicsQueue().queue.presentKHR(presentInfo);

        mCurrentFrameIndex  = (mCurrentFrameIndex + 1) % conf::gFramesInFlight;
        mLifetimeFrameCount += 1;
    }

    bool VulkanRHI::isFrameComplete(const uint64_t frame) const
    {
        return mGraphicsQueue->getTimeline()->isComplete(frame);
    }

    void VulkanRHI::queueDeletion(UPtr<Resource> resource)
    {
        mPendingDeletes.push_back({
            .resource      = std::move(resource),
            .deletionFrame = mLifetimeFrameCount + conf::gFramesInFlight,
        });
    }

    UPtr<Texture> VulkanRHI::createTexture(const TextureCreateInfo& textureInfo)
    {
        return VulkanTexture::create(textureInfo, mDevice);
    }

    ICommandQueue* VulkanRHI::getGraphicsQueue() const
    {
        return mGraphicsQueue.get();
    }

    ICommandQueue* VulkanRHI::getComputeQueue() const
    {
        return mComputeQueue.get();
    }

    void VulkanRHI::flushPendingDeletes()
    {
        std::erase_if(mPendingDeletes, [&](const PendingDelete& p) {
            return mLifetimeFrameCount >= p.deletionFrame;
        });
    }
}
