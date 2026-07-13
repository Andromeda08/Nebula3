#include "DLSS.hpp"

#include "VulkanRHI/Commands/CommandList.hpp"

namespace RHI::Integration
{
    DLSS::DLSS(const SPtr<Instance>& instance,
              const SPtr<Device>&    device,
              CommandQueue*          queue)
    : mInstance(instance)
    , mDevice(device)
    , mQueue(queue)
    {
        if (device->getDeviceExtensions().getProperties().vendorID != 0x10DE)
        {
            spdlog::error("DLSS is only available for NVIDIA GPUs");
            return;
        }
        if (!checkFeatures())
        {
            spdlog::error("Not all feature requirements were met for DLSS support.");
            return;
        }

        #ifdef nbl_DLSS_AVAILABLE
        initNGX();
        #endif
    }

    DLSS::~DLSS()
    {
        #ifdef nbl_DLSS_AVAILABLE
        if (mDLSSDenoiserFeature)
        {
            NVSDK_NGX_VULKAN_ReleaseFeature(mDLSSDenoiserFeature);
        }
        #endif
    }

    void DLSS::evalDLSSDenoiser(const CommandList* pCommandList, NVSDK_NGX_VK_DLSSD_Eval_Params *eval) const
    {
        if (!mDLSSDenoiserFeature)
        {
            spdlog::error("DLSS Denoiser feature was not initialized or is not available.");
            return;
        }

        #ifdef nbl_DLSS_AVAILABLE
        const auto r = NGX_VULKAN_EVALUATE_DLSSD_EXT(pCommandList->getHandle(), mDLSSDenoiserFeature, mNgxParams, eval);
        if (NVSDK_NGX_FAILED(r))
        {
            spdlog::error("DLSS Denoiser eval failed: 0x{:x}", static_cast<uint32_t>(r));
        }
        #endif
    }

    NVSDK_NGX_Parameter* DLSS::getNGXParams() const
    {
        return mNgxParams;
    }

    void DLSS::initNGX()
    {
        #ifdef nbl_DLSS_AVAILABLE

        NVSDK_NGX_Result r = NVSDK_NGX_VULKAN_Init_with_ProjectID(
            "1b43cbbc-2a8e-419a-a5b4-6c938e1b4086",
            NVSDK_NGX_ENGINE_TYPE_CUSTOM,
            "1.0",
            L"./nv/",
            mInstance->getHandle(),
            mDevice->getPhysicalDevice(),
            mDevice->getHandle(),
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
            nullptr,
            NVSDK_NGX_Version_API);

        if (NVSDK_NGX_FAILED(r))
        {
            spdlog::error("Failed to init NGX: 0x{:x}", static_cast<uint32_t>(r));
            return;
        }

        r = NVSDK_NGX_VULKAN_GetCapabilityParameters(&mNgxParams);
        if (NVSDK_NGX_FAILED(r))
        {
            spdlog::error("Failed to get capabilities for NGX: 0x{:x}", static_cast<uint32_t>(r));
            return;
        }

        int available = 0;
        mNgxParams->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &available);

        if (!available)
        {
            int needsUpdatedDriver = 0;
            mNgxParams->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_NeedsUpdatedDriver, &needsUpdatedDriver);
            spdlog::error("DLSS Denoising is unavailable (driver update needed: {})", needsUpdatedDriver);
            return;
        }

        mDLSSDenoiserAvailable = true;

        #endif
    }

    void DLSS::createDLSSDenoiserFeature(const DLSS_DenoiserParams& params)
    {
        if (!mDLSSDenoiserAvailable)
        {
            exitWithError("DLSS Denoiser is not available.");
        }

        #ifdef nbl_DLSS_AVAILABLE

        if (mDLSSDenoiserFeature)
        {
            NVSDK_NGX_VULKAN_ReleaseFeature(mDLSSDenoiserFeature);
            mDLSSDenoiserFeature = nullptr;
        }

        NVSDK_NGX_DLSSD_Create_Params p = {};
        p.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
        p.InRoughnessMode    = NVSDK_NGX_DLSS_Roughness_Mode_Unpacked;
        p.InUseHWDepth       = NVSDK_NGX_DLSS_Depth_Type_Linear;
        p.InWidth            = params.inExtent.width;
        p.InHeight           = params.inExtent.height;
        p.InTargetWidth      = params.targetExtent.width;
        p.InTargetHeight     = params.targetExtent.height;
        p.InPerfQualityValue = params.quality;
        p.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR
            | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes
            | NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

        NVSDK_NGX_Result r {};
        mQueue->immediate([&](const CommandList* pCommandList) -> void
        {
            r = NGX_VULKAN_CREATE_DLSSD_EXT1(mDevice->getHandle(), pCommandList->getHandle(),
            1, 1, &mDLSSDenoiserFeature, mNgxParams, &p);
        });

        if (NVSDK_NGX_FAILED(r))
        {
            exitWithError("DLSS Denoiser feature creation failed: 0x{:x}", static_cast<uint32_t>(r));
        }

        #endif
    }

    NVSDK_NGX_Handle* DLSS::getDLSSDenoiserFeature(const std::optional<DLSS_DenoiserParams>& denoiserParams)
    {
        if (!mDLSSDenoiserAvailable)
        {
            exitWithError("DLSS Denoiser is not available.");
        }

        #ifdef nbl_DLSS_AVAILABLE
        if (!mDLSSDenoiserFeature || denoiserParams.has_value())
        {
            createDLSSDenoiserFeature(denoiserParams.value());
        }
        #endif

        return mDLSSDenoiserFeature;
    }

    bool DLSS::isAvailable() const
    {
        return mDLSSDenoiserAvailable;
    }

    void DLSS::addRequiredDeviceExtensions(DeviceExtensions& extensions)
    {
        #ifdef nbl_DLSS_AVAILABLE
        uint32_t     ngxVoid0 = 0,         ngxDeviceCount = 0;
        const char** ngxVoid1 = nullptr, **ngxDevExts     = nullptr;
        NVSDK_NGX_VULKAN_RequiredExtensions(&ngxVoid0, &ngxVoid1, &ngxDeviceCount, &ngxDevExts);
        for (uint32_t i = 0; i < ngxDeviceCount; i++)
        {
            extensions.addExtension(ngxDevExts[i], Platform::isApple ? FeatureOption::Optional : FeatureOption::Required);
        }
        #endif
    }

    NVSDK_NGX_Resource_VK DLSS::wrapImage(const SPtr<Image>& image, const bool readWrite)
    {
        #ifdef nbl_DLSS_AVAILABLE
        const auto& p = image->getProperties();

        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        return NVSDK_NGX_Create_ImageView_Resource_VK(
            static_cast<VkImageView>(image->getImageView()),
            static_cast<VkImage>(image->getImage()),
            range,
            static_cast<VkFormat>(p.format),
            p.extent.width,
            p.extent.height,
            readWrite);
        #else
        return {};
        #endif
    }

    bool DLSS::checkFeatures() const
    {
        #ifdef nbl_DLSS_AVAILABLE
        uint32_t     ngxVoid0 = 0,         ngxDeviceCount = 0;
        const char** ngxVoid1 = nullptr, **ngxDevExts     = nullptr;
        NVSDK_NGX_VULKAN_RequiredExtensions(&ngxVoid0, &ngxVoid1, &ngxDeviceCount, &ngxDevExts);

        bool ok = true;
        for (uint32_t i = 0; i < ngxDeviceCount; i++)
        {
            if (!contains(mDevice->getDeviceExtensions().getActiveExtensionNames(), ngxDevExts[i]))
            {
                ok = false;
            }
        }
        return ok;
        #else
        return false;
        #endif
    }
}
