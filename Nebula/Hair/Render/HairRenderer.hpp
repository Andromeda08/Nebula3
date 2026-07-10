#pragma once

#include <glm/glm.hpp>

#include "HairRenderingMode.hpp"
#include "Core/Types.hpp"
#include "Hair/HairGeometry.hpp"
#include "Level/Transform.hpp"
#include "Level/Render/Templates.hpp"
#include "UserInterface/IComponent.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HairRenderer
    {
        struct PushConstants
        {
            // Instance Data
            glm::mat4 model;
            glm::vec4 diffuse;
            glm::vec4 specular;
            // Buffer References
            uint64_t  vertexBufferAddress;
            uint64_t  attributesBufferAddress;
            uint64_t  strandDescBufferAddress;
            uint64_t  debugColorBufferAddress;
            uint64_t  cameraBufferAddress;
            // Geometry Params
            uint32_t  firstVertex;
            uint32_t  vertexCount;
            uint32_t  firstStrand;
            uint32_t  strandCount;
            // Renderer Config
            uint32_t  renderMode;
            int32_t   useCustomColors;
            float     specularFactor;
            int32_t   _pad0;
        };

    public:
        HairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModels);

        void render(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, uint32_t hairIndex, uint64_t cameraBuffer);

        const SPtr<RHI::Image>& getResult(const uint32_t frameIndex) const
        {
            return mRenderTarget[frameIndex];
        }

    private:
        friend class HairRendererUI;

        void createDebugColors();

        void createResources();

        void createPipeline();

        SPtr<RHI::VulkanRHI>                    mRHI;
        HairModelSystem*                        mHairModels;

        HairRenderingMode                       mRenderingMode = HairRenderingMode::Default;

        int32_t                                 mHairIndex        = 0;
        float                                   mSpecularFactor   = 16.0f;
        bool                                    mUseCustomColor   = false;
        glm::vec4                               mDiffuse          = glm::vec4(0.32549f, 0.23921f, 0.20784f, 1.0f);
        glm::vec4                               mSpecular         = glm::vec4(0.41568f, 0.30588f, 0.21960f, 1.0f);
        bool                                    mUseCustomWgSize  = false;
        int32_t                                 mCustomTaskWgSize = 0;

        std::array<glm::vec4, 1024>             mDebugColors;
        SPtr<RHI::Buffer>                       mDebugColorsBuffer;

        vk::Rect2D                              mScissor;
        vk::Viewport                            mViewport;
        SPtr<RHI::Pipeline>                     mPipeline;
        PerFrameArray<SPtr<RHI::Image>>         mRenderTarget;
        PerFrameArray<SPtr<RHI::Image>>         mDepthBuffer;
    };

    class HairRendererUI : public IComponent
    {
    public:
        explicit HairRendererUI(HairRenderer* pHairRenderer)
        : IComponent()
        , mHairRenderer(pHairRenderer)
        {
        }

        void draw() override;

    private:
        HairRenderer* mHairRenderer;
    };
}
