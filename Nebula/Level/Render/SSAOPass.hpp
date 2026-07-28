#pragma once

#include <glm/glm.hpp>
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class SSAOPass
    {
        struct PushConstant
        {
            uint64_t    cameraBuffer = 0;
            uint64_t    kernelBuffer = 0;
            glm::ivec2  renderDim;
            glm::ivec2  noiseDim;
        };
    public:
        struct Input
        {
            SPtr<RHI::Image> positions;
            SPtr<RHI::Image> normals;
            SPtr<RHI::Image> viewZ;
            uint64_t         cameraBuffer;
        };

        nbl_DisableCopy(SSAOPass);
        nbl_CreateWithStruct(SSAOPass, SPtr<RHI::VulkanRHI>);

        explicit SSAOPass(const SPtr<RHI::VulkanRHI>& rhi);

        ~SSAOPass() = default;

        void execute(const Input& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] const SPtr<RHI::Image>& getResult(uint32_t currentFrame) const noexcept;

        [[nodiscard]] const PerFrameArray<SPtr<RHI::Image>>& getResults() const noexcept;

    private:
        void createKernel() noexcept;
        void createNoiseTexture() noexcept;

        void createResources_SSAO() noexcept;
        void createResources_Blur() noexcept;

        void execute_SSAO(const Input& input, RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;
        void execute_Blur(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        constexpr static uint32_t sKernelSize = 64;
        constexpr static uint32_t sNoiseSize  = 8;
        constexpr static float    sRadius     = 1.0f;

        bool mRunBlurPass = true;

        SPtr<RHI::VulkanRHI>            mRHI;

        // SSAO Pass
        // ============================
        PerFrameArray<SPtr<RHI::Image>> mSSAO_Result;
        SPtr<RHI::Image>                mSSAO_Noise;
        SPtr<RHI::Buffer>               mSSAO_Kernel;

        SPtr<RHI::GraphicsPipeline2>    mSSAO_Pipeline;
        SPtr<RHI::Descriptor>           mSSAO_Descriptor;

        // SSAO Blur Pass
        // ============================
        PerFrameArray<SPtr<RHI::Image>> mBlur_Result;

        SPtr<RHI::GraphicsPipeline2>    mBlur_Pipeline;
        SPtr<RHI::Descriptor>           mBlur_Descriptor;
    };

}