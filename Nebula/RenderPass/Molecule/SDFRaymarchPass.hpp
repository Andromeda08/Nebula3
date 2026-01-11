#pragma once

#include <glm/glm.hpp>

#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
    struct SDFRaymarchParams
    {
        glm::vec4 bboxMin;
        glm::vec4 bboxMax;
        glm::vec4 sesColor;
        float voxelSize;
        float blending;
        float ls;
        int useSubsurfaceScattering;
        int rayMarchingSteps;
    };

    class SDFRaymarchPass
    {
    public:
        SDFRaymarchPass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Descriptor>& sceneDescriptor, const SPtr<RHI::Texture>& sdfTexture);

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const;

        void setParams(const SDFRaymarchParams& params) noexcept
        {
            mParams = params;
        }

    private:
        friend class MoleculeRenderingUI;

        void createSampler();

        SPtr<RHI::VulkanRHI>        mRHI;
        vk::Sampler                 mSampler;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::GraphicsPipeline> mPipeline;
        SPtr<RHI::RenderPass>       mRenderPass;

        SPtr<RHI::Texture>          mSDFTexture;
        SPtr<RHI::Descriptor>       mSceneDescriptor;

        SDFRaymarchParams           mParams = {};
    };
}