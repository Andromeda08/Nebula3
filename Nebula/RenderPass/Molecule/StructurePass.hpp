#pragma once

#include <glm/glm.hpp>

#include "Scene/Molecule/CIFData.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
    struct StructurePassParams
    {
        glm::vec4 structureColor = { 0.45f, 0.2f, 0.8f, 1.0f };
    };

    class StructurePass
    {
    public:
        StructurePass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Descriptor>& sceneDescriptor, CIFData* pCIFData);

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const;

    private:
        friend class MoleculeRenderingUI;

        CIFData*                    mCIFData;

        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Image>            mDepthImage;
        SPtr<RHI::GraphicsPipeline> mPipeline;
        SPtr<RHI::RenderPass>       mRenderPass;

        SPtr<RHI::Descriptor>       mSceneDescriptor;

        StructurePassParams         mParams;
    };
}
