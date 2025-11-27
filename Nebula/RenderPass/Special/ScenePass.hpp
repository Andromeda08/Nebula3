#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"

namespace rg
{
    class ScenePass final : public IPass
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
                    DependencyInfo {
                        .name           = "Top-level AS",
                        .dependencyType = DependencyType::Expose,
                        .resourceType   = ResourceType::TopLevelAS,
                    },
                },
            };
        }
    };
}
