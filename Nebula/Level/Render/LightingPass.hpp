#pragma once

#include "Core/Types.hpp"
#include "Level/Level.hpp"
#include "../TextureManager.hpp"
#include "UserInterface/UserInterface.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;
    class GBufferPass;

    struct LightingPass_Params
    {
        SPtr<GBufferPass>    pGBufferPass    = nullptr;
        TextureManager*      pTextureManager = nullptr;
        Level*               pLevel          = nullptr;
        SPtr<RHI::VulkanRHI> rhi             = nullptr;
    };

    /**
     * Lighting Pass
     */
    class LightingPass
    {
        struct PushConstants
        {
            uint64_t    instances;
            uint64_t    instanceIndirectionMap;
            uint64_t    camera;
            uint64_t    materials;
            uint64_t    lights;
            uint64_t    vertexBuffer;
            uint64_t    indexBuffer;
            uint64_t    geometryBuffer;
            int32_t     shadowsEnabled;
            int32_t     sampleCount;
            int32_t     enableGI;
            float       ambientFactor;
            float       shadowFactor;
            float       emissiveFactor;
        };
    public:
        nbl_DisableCopy(LightingPass);
        nbl_CreateWithStruct(LightingPass, LightingPass_Params);

        explicit LightingPass(const LightingPass_Params& params);

        ~LightingPass() = default;

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData) const noexcept;

        [[nodiscard]] const SPtr<RHI::Image>& getResult() const noexcept
        {
            return mLightingResult;
        }

    private:
        friend class LightingPassUI;

        void init() noexcept;

        SPtr<RHI::VulkanRHI>        mRHI;

        bool                        mEnableShadows  = true;
        bool                        mEnableGI       = true;
        int32_t                     mSampleCount    = 8;
        float                       mAmbientFactor  = 0.05f;
        float                       mShadowFactor   = 0.1f;
        float                       mEmissiveFactor = 5.0f;

        LightingPass_Params         mInput;

        vk::Rect2D                  mScissor;
        vk::Viewport                mViewport;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::RenderPass>       mRenderPass;
        SPtr<RHI::Pipeline>         mPipeline;

        SPtr<RHI::Image>            mLightingResult;
    };

    class LightingPassUI : public IComponent
    {
    public:
        explicit LightingPassUI(LightingPass* pPass);

        void draw() override;

    private:
        LightingPass* mPass;
    };
}
