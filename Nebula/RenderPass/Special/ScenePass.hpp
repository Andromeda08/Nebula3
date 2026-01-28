#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class ScenePass final : public Pass
    {
    public:
        ~ScenePass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::Scene,
                .displayName  = "Source",
                .dependencies = {
                    DependencyInfo {
                        .name           = "Scene Data",
                        .dependencyType = DependencyType::Expose,
                        .resourceType   = ResourceType::SceneData,
                    },
                },
            };
        }
    };
}
