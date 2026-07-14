#pragma once

#include <glm/glm.hpp>

#include "Core/Types.hpp"
#include "VulkanRHI/Image.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    struct PrePass_Params
    {
        Level*               pLevel;
        SPtr<RHI::VulkanRHI> rhi;
    };

    class [[nodiscard]] PrePass
    {
        struct PushConstants
        {
            uint64_t instanceBuffer;
            uint64_t instanceIndirectionBuffer;
            uint64_t cameraBuffer;
            uint64_t materialBuffer;
        };
    public:
        nbl_DisableCopy(PrePass);
        nbl_CreateWithStruct(PrePass, PrePass_Params);

        explicit PrePass(const PrePass_Params& params);

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const;

        const SPtr<RHI::Image>& getDepthBuffer(uint32_t currentFrame) const;

        const SPtr<RHI::Image>& getObjInstanceBuffer(uint32_t currentFrame) const;

    private:
        SPtr<RHI::VulkanRHI>            mRHI;
        Level*                          mLevel;

        PerFrameArray<SPtr<RHI::Image>> mDepthBuffer;       // D32  Float
        PerFrameArray<SPtr<RHI::Image>> mObjInstanceBuffer; // RG32 SInt    [ Obj ID, Instance ID ]

        UPtr<RHI::GraphicsPipeline2>    mPipeline;
    };
}
