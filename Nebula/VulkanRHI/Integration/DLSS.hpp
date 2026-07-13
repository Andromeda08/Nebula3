#pragma once

#include <optional>

#include <vulkan/vulkan.hpp>

#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>
#include <nvsdk_ngx_helpers_dlssd_vk.h>


#include "Core/Types.hpp"
#include "VulkanRHI/Device.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/Instance.hpp"
#include "VulkanRHI/Commands/CommandQueue.hpp"

namespace RHI::Integration
{
    struct DLSS_DenoiserParams
    {
        vk::Extent2D                inExtent        = {0, 0};
        vk::Extent2D                targetExtent    = {0, 0};
        NVSDK_NGX_PerfQuality_Value quality         = NVSDK_NGX_PerfQuality_Value_DLAA;
    };

    class [[nodiscard]] DLSS
    {
    public:
        DLSS(const SPtr<Instance>& instance,
             const SPtr<Device>&   device,
             CommandQueue*         queue);

        ~DLSS();

        void evalDLSSDenoiser(const CommandList* pCommandList, NVSDK_NGX_VK_DLSSD_Eval_Params *eval) const;

        NVSDK_NGX_Parameter* getNGXParams() const;

        NVSDK_NGX_Handle* getDLSSDenoiserFeature(const std::optional<DLSS_DenoiserParams>& denoiserParams);

        bool isAvailable() const;

        static void addRequiredDeviceExtensions(DeviceExtensions& extensions);

        static NVSDK_NGX_Resource_VK wrapImage(const SPtr<Image>& image, bool readWrite);

    private:
        bool checkFeatures() const;

        void initNGX();

        void createDLSSDenoiserFeature(const DLSS_DenoiserParams& params);

        SPtr<Instance>  mInstance   = nullptr;
        SPtr<Device>    mDevice     = nullptr;
        CommandQueue*   mQueue      = nullptr;

        NVSDK_NGX_Parameter*    mNgxParams              = nullptr;
        bool                    mDLSSDenoiserAvailable  = false;
        NVSDK_NGX_Handle*       mDLSSDenoiserFeature    = nullptr;
    };
}
