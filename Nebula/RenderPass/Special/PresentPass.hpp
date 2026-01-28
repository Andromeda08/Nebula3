#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class PresentPass final : public Pass
    {
    public:
        ~PresentPass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::Present,
                .displayName  = "Present",
                .dependencies = {
                    DependencyInfo {
                        .name           = "PresentSrc",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                },
            };
        }
    };
}
