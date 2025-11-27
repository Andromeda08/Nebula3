#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"

namespace rg
{
    class LightingPass final : public IPass
    {
    public:
        ~LightingPass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::LightingPass,
                .displayName  = "Lighting Pass",
                .dependencies = {
                    DependencyInfo {
                        .name           = "Top-level AS",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::TopLevelAS,
                    },
                    DependencyInfo {
                        .name           = "Position Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::ShaderReadOnly,
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::ShaderReadOnly,
                    },
                    DependencyInfo {
                        .name           = "Albedo Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::ShaderReadOnly,
                    },
                    DependencyInfo {
                        .name           = "Lighting Result",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::ColorAttachment,
                    },
                },
            };
        }
    };
}
