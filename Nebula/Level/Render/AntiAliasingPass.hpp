#pragma once

#include "VulkanRHI/VulkanRHI.hpp"
#include "VulkanRHI/Render/Pipeline.hpp"

namespace nbl
{
    struct AntiAliasing_Params
    {
        PerFrameArray<SPtr<RHI::Image>> input;
        SPtr<RHI::VulkanRHI>            rhi;
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

        [[nodiscard]] const SPtr<RHI::Image>& getResult(uint32_t currentFrame) const noexcept;

        [[nodiscard]] const PerFrameArray<SPtr<RHI::Image>>& getResults() const noexcept;

    private:
        void createResources() noexcept;
        void createPipeline()  noexcept;

        SPtr<RHI::VulkanRHI>         mRHI;

        PerFrameArray<SPtr<RHI::Image>> mInput;
        PerFrameArray<SPtr<RHI::Image>> mOutput;

        SPtr<RHI::Descriptor>        mDescriptor;
        SPtr<RHI::GraphicsPipeline2> mPipeline;
    };

}
