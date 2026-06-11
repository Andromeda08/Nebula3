#pragma once

#include "Core/Types.hpp"
#include "Level/Level.hpp"
#include "../TextureManager.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    struct GBufferPass_Params
    {
        TextureManager*      pTextureManager = nullptr;
        Level*               pLevel          = nullptr;
        SPtr<RHI::VulkanRHI> rhi             = nullptr;
    };

    /**
     * G-Buffer Pass
     */
    class GBufferPass
    {
        struct PushConstants
        {
            uint64_t   instanceBuffer;
            uint64_t   instanceIndirectionBuffer;
            uint64_t   cameraBuffer;
            uint64_t   previousCameraBuffer;
            uint64_t   materialBuffer;
            glm::uvec2 renderRes;
        };
    public:
        nbl_DisableCopy(GBufferPass);
        nbl_CreateWithStructShared(GBufferPass, GBufferPass_Params);

        explicit GBufferPass(const GBufferPass_Params& params);

        ~GBufferPass() = default;

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] const SPtr<RHI::Image>& getAlbedoBuffer() const noexcept;

        [[nodiscard]] const SPtr<RHI::Image>& getDepthBuffer() const noexcept;

    private:
        void init() noexcept;

        SPtr<RHI::VulkanRHI>        mRHI;

        GBufferPass_Params          mInput;

        vk::Rect2D                  mScissor;
        vk::Viewport                mViewport;

        SPtr<RHI::RenderPass>       mRenderPass;
        SPtr<RHI::Pipeline>         mPipeline;

    public:
        SPtr<RHI::Image>            mWorldPosition;     // RGBA 16 Float
        SPtr<RHI::Image>            mWorldNormal;       // RGBA 16 Float
        SPtr<RHI::Image>            mMotionVectors;     // RGBA 32 Float [XY pixel, Z lin. depth delta, W unused]
        SPtr<RHI::Image>            mViewZ;             // R    32 Float
        SPtr<RHI::Image>            mAlbedoBuffer;      // RGBA 16 Float [XYZ Color, W alpha clip]
        SPtr<RHI::Image>            mEmissiveBuffer;    // R     8 UInt
        SPtr<RHI::Image>            mParamsBuffer;      // RG   16 Float [X Metallic, Y Roughness]
        SPtr<RHI::Image>            mDepthBuffer;       // D    32 Float
    };
}
