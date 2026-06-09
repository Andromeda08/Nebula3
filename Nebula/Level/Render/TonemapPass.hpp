#pragma once

#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    struct Tonemap_Params
    {
        SPtr<RHI::Image>     color;
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

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept;

    private:
        friend class SceneInfoComponent;

        void createResources() noexcept;
        void createPipeline()  noexcept;

        PushConstant                    mPushConstant;

        SPtr<RHI::VulkanRHI>            mRHI;

        SPtr<RHI::Image>                mInput;
        SPtr<RHI::Image>                mOutput;

        SPtr<RHI::Descriptor>           mDescriptor;
        SPtr<RHI::GraphicsPipeline2>    mPipeline;
    };

}