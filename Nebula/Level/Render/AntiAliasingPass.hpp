#pragma once

#include "VulkanRHI/VulkanRHI.hpp"
#include "VulkanRHI/Render/Pipeline.hpp"

namespace nbl
{
    struct AntiAliasing_Params
    {
        SPtr<RHI::Image>     input;
        SPtr<RHI::VulkanRHI> rhi;
    };

    class AntiAliasingPass
    {
        struct PushConstant
        {
            float rcpX;
            float rcpY;
        };
    public:
        nbl_DisableCopy(AntiAliasingPass);
        nbl_CreateWithStruct(AntiAliasingPass, AntiAliasing_Params);

        explicit AntiAliasingPass(const AntiAliasing_Params& params);

        ~AntiAliasingPass() = default;

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] SPtr<RHI::Image> getResult() const noexcept;

    private:
        void createResources() noexcept;
        void createPipeline()  noexcept;

        SPtr<RHI::VulkanRHI>         mRHI;

        SPtr<RHI::Image>             mInput;
        SPtr<RHI::Image>             mOutput;

        SPtr<RHI::Descriptor>        mDescriptor;
        SPtr<RHI::GraphicsPipeline2> mPipeline;
    };

}
