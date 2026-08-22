#pragma once

#include <rhi/DynamicRHI.hpp>
#include <rhi/IWindow.hpp>

#include <rhi/vulkan/Device.hpp>
#include <rhi/vulkan/VulkanCommon.hpp>
#include <rhi/vulkan/detail/Instance.hpp>
#include <rhi/vulkan/detail/Surface.hpp>

#include "CommandQueue.hpp"
#include "Swapchain.hpp"
#include "detail/VulkanTimelineSync.hpp"

/**
 * Vulkan RHI targeting bindless-first GPU-driven setups.
 */
namespace sunflower::rhi
{
    struct VulkanRHICreateInfo
    {
        IWindow*        pWindow         = nullptr;
        Option<String>  applicationName = {};
        Option<String>  engineName      = {};
    };

    class VulkanRHI final : public DynamicRHI
    {
    public:
        sunflower_DisableCopy(VulkanRHI);
        sunflower_Create(VulkanRHI, SPtr);

        ~VulkanRHI() override;

        FrameInfo beginFrame() override;

        void endFrame_submitAndPresent(const PresentFrameInfo& presentFrameInfo) override;

        bool isFrameComplete(uint64_t frame) const override;

        void queueDeletion(UPtr<Resource> resource) override;

        [[nodiscard]] UPtr<Texture> createTexture(const TextureCreateInfo& textureInfo) override;

        ICommandQueue* getGraphicsQueue() const override;

        ICommandQueue* getComputeQueue() const override;

    private:
        void flushPendingDeletes();

        IWindow*               mWindow;
        SPtr<detail::Instance> mInstance;
        UPtr<detail::Surface>  mSurface;
        SPtr<Device>           mDevice;
        UPtr<VulkanSwapchain>  mSwapchain;

        UPtr<VulkanCommandQueue>        mGraphicsQueue;
        UPtr<VulkanCommandQueue>        mComputeQueue;

        // Binary semaphores for Swapchain sync
        PerFrameArray<vk::Semaphore>    mImageReady;
        PerFrameArray<vk::Semaphore>    mRenderingFinished;
        uint32_t                        mAcquiredImageIndex = 0;
        uint64_t                        mCurrentFrameIndex  = 0;
        uint64_t                        mLifetimeFrameCount = 0;

        std::vector<PendingDelete>      mPendingDeletes;
    };
}