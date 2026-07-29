#pragma once

#include <glm/glm.hpp>

#include "HairRenderingMode.hpp"
#include "IHairRenderer.hpp"
#include "Core/Random.hpp"
#include "Core/Types.hpp"
#include "Hair/HairGeometry.hpp"
#include "Level/Transform.hpp"
#include "Level/Render/Templates.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class ClassicHairRenderer : public IHairRenderer
    {
        struct PushConstants
        {
            // Instance Data
            glm::mat4 model;
            glm::vec4 diffuse;
            glm::vec4 specular;
            // Buffer References
            uint64_t  vertexBufferAddress;
            // uint64_t  attributesBufferAddress;
            uint64_t  strandDescBufferAddress;
            uint64_t  debugColorBufferAddress;
            uint64_t  cameraBufferAddress;
            // Geometry Params
            uint32_t  firstVertex;
            uint32_t  vertexCount;
            uint32_t  firstStrand;
            uint32_t  strandCount;
        };

    public:
        ClassicHairRenderer(const SPtr<RHI::VulkanRHI>& rhi, HairModelSystem* pHairModels);

        ~ClassicHairRenderer() override = default;

        void execute(RHI::CommandList* pCommandList, const RHI::FrameData& frameData, uint64_t cameraBufferAddress) override;

        const SPtr<RHI::Image>& getResult(uint32_t frameIndex) const override;

    private:
        void createDebugColors();

        void createResources();

        void createPipeline();

        SPtr<RHI::VulkanRHI>                    mRHI;
        HairModelSystem*                        mHairModels;

        HairRenderingMode                       mRenderingMode = HairRenderingMode::Default;

        std::array<glm::vec4, 1024>             mDebugColors {};
        SPtr<RHI::Buffer>                       mDebugColorsBuffer;

        vk::Rect2D                              mScissor;
        vk::Viewport                            mViewport;
        SPtr<RHI::Pipeline>                     mPipeline;
        PerFrameArray<SPtr<RHI::Image>>         mRenderTarget;
        PerFrameArray<SPtr<RHI::Image>>         mDepthBuffer;
    };
}
