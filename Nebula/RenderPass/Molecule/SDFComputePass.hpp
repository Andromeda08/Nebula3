#pragma once

#include <glm/glm.hpp>

#include "VulkanRHI/Rendering.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace Molecule
{
    class SDFComputePass
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

        void execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData) const;

        [[nodiscard]] SPtr<RHI::Image3D> getSDFTexture3D() const noexcept;

        ~SDFComputePass() = default;

        const PushConstants& getPushConstants() const noexcept
        {
            return mPushConstants;
        }

    private:
        friend class MoleculeRenderingUI;

        SPtr<RHI::VulkanRHI>        mRHI;
        SPtr<RHI::Descriptor>       mDescriptor;
        SPtr<RHI::ComputePipeline>  mPipeline;
        std::array<uint32_t, 3>     mDispatchSize;

        SPtr<RHI::Image3D>          mImage3D;
        SPtr<RHI::Buffer>           mPositions;
        SPtr<RHI::Buffer>           mConfig;

        PushConstants               mPushConstants;
        vk::Extent3D                mTextureExtent;
    };
}
