#pragma once

#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct Tonemap_Params
    {
        vk::Format           outputFormat;
        SPtr<RHI::VulkanRHI> rhi;
    };

    class TonemapPass
    {
        struct PushConstant
        {
            float exposure = 1.0;
        };
    public:
        nbl_DisableCopy(TonemapPass);
        nbl_CreateWithStruct(TonemapPass, Tonemap_Params);

        explicit TonemapPass(const Tonemap_Params& params);

        ~TonemapPass() = default;

        void execute(const SPtr<RHI::Image>& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] const SPtr<RHI::Image>& getResult(uint32_t currentFrame) const noexcept;

        [[nodiscard]] const PerFrameArray<SPtr<RHI::Image>>& getResults() const noexcept;

    private:
        friend class SceneInfoComponent;

        SPtr<RHI::VulkanRHI>            mRHI;
        SPtr<RHI::Descriptor>           mDescriptor;
        SPtr<RHI::GraphicsPipeline2>    mPipeline;
        PerFrameArray<SPtr<RHI::Image>> mOutput;
        PushConstant                    mPushConstant;
    };

}