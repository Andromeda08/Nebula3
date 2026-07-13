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

        NVSDK_NGX_Parameter* getNGXParams() const noexcept { return mNgxParams; }
        NVSDK_NGX_Handle*    getDLSSdFeature() const noexcept { return mDLSSdFeature; }

    private:
        void initDLSS()
        {
            NVSDK_NGX_Result r = NVSDK_NGX_VULKAN_Init_with_ProjectID(
                "1b43cbbc-2a8e-419a-a5b4-6c938e1b4086",
                NVSDK_NGX_ENGINE_TYPE_CUSTOM,
                "1.0",
                L"./ngxLogs/",
                mInstance->getHandle(),
                mDevice->getPhysicalDevice(),
                mDevice->getHandle(),
                VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
                VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
                nullptr,
                NVSDK_NGX_Version_API);

            if (NVSDK_NGX_FAILED(r))
            {
                spdlog::warn("NGX init failed: 0x{:x}", static_cast<uint32_t>(r));
                return;
            }

            r = NVSDK_NGX_VULKAN_GetCapabilityParameters(&mNgxParams);
            if (NVSDK_NGX_FAILED(r))
            {
                spdlog::warn("NGX failed to get capabilities: 0x{:x}", static_cast<uint32_t>(r));
                return;
            }

            int available = 0;
            mNgxParams->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &available);

            if (!available)
            {
                int needsUpdatedDriver = 0;
                mNgxParams->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_NeedsUpdatedDriver, &needsUpdatedDriver);
                spdlog::warn("DLSS-RR unavailable (driver update needed: {})", needsUpdatedDriver);
                return;
            }

            mRRAvailable = true;

            if (!mRRAvailable)
            {
                return;
            }
            if (mDLSSdFeature)
            {
                NVSDK_NGX_VULKAN_ReleaseFeature(mDLSSdFeature);
                mDLSSdFeature = nullptr;
            }

            NVSDK_NGX_DLSSD_Create_Params p{};
            p.InDenoiseMode      = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
            p.InRoughnessMode    = NVSDK_NGX_DLSS_Roughness_Mode_Unpacked;
            p.InUseHWDepth       = NVSDK_NGX_DLSS_Depth_Type_Linear;
            p.InWidth            = mSwapchain->getProperties().extent.width;
            p.InHeight           = mSwapchain->getProperties().extent.height;
            p.InTargetWidth      = mSwapchain->getProperties().extent.width;
            p.InTargetHeight     = mSwapchain->getProperties().extent.height;
            p.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_DLAA;
            p.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR
                | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes
                | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
                // | NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;

            getGraphicsQueue()->immediate([&](const RHI::CommandList* cmd) {
                r = NGX_VULKAN_CREATE_DLSSD_EXT1(
                    getDevice()->getHandle(),
                    cmd->getHandle(),
                    1, 1,
                    &mDLSSdFeature,
                    mNgxParams,
                    &p);

                if (NVSDK_NGX_FAILED(r))
                {
                    exitWithError("DLSS-RR feature creation failed: 0x{:x}", static_cast<uint32_t>(r));
                }
            });

            spdlog::info("DLSS-RR is available.");
        }

        SPtr<IWindow>               mWindow;

        SPtr<Instance>              mInstance;
        UPtr<DebugContext>          mDebugContext;
        SPtr<Device>                mDevice;
        UPtr<Swapchain>             mSwapchain;
        UPtr<CommandQueue>          mGraphicsQueue;
        UPtr<FrameSync>             mFrameSync;

        std::vector<PendingDelete>  mDeletionQueue;

        NVSDK_NGX_Parameter* mNgxParams     = nullptr;
        NVSDK_NGX_Handle*    mDLSSdFeature  = nullptr;
        bool                 mRRAvailable   = false;
    };
}
