#pragma once

#include <glm/glm.hpp>

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"
#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
    class SDFComputePass : public IPass
    {
    public:
        struct PushConstants
        {
            glm::vec4 bboxMin;
            glm::vec4 bboxMax;
            int32_t   nAtoms;
            float     radius;
            float     scale;
        };

        SDFComputePass(SPtr<RHI::VulkanRHI> rhi, const std::vector<glm::vec3>& atomPositions);

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) override;

        [[nodiscard]] SPtr<RHI::Texture> getSDFTexture3D() const noexcept;

        ~SDFComputePass() override = default;

        const PushConstants& getPushConstants() const noexcept
        {
            return mPushConstants;
        }

        static rg::NodeCreateInfo getNodeInfo() noexcept;

    private:
        friend class MoleculeRenderingUI;

        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::ComputePipeline>  mPipeline;
        std::array<uint32_t, 3>     mDispatchSize;

        SPtr<RHI::Texture>          mImage3D;
        SPtr<RHI::Buffer>           mPositions;
        SPtr<RHI::Buffer>           mConfig;

        PushConstants               mPushConstants;
        vk::Extent3D                mTextureExtent;
    };
}
