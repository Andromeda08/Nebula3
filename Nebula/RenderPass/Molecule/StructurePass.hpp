#pragma once

#include <glm/glm.hpp>

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"
#include "Scene/Molecule/CIFData.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
    struct StructurePassParams
    {
        glm::vec4 structureColor = { 0.45f, 0.2f, 0.8f, 1.0f };
    };

    class StructurePass : public Pass
    {
    public:
        ~StructurePass() override = default;

        StructurePass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Descriptor>& sceneDescriptor, CIFData* pCIFData);

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) override;

        static rg::NodeCreateInfo getNodeInfo() noexcept;

        CIFData*                    mCIFData;

        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Image>            mDepthImage;
        SPtr<RHI::GraphicsPipeline> mPipeline;
        SPtr<RHI::RenderPass>       mRenderPass;

        SPtr<RHI::Descriptor>       mSceneDescriptor;

        StructurePassParams         mParams;
    };
}
