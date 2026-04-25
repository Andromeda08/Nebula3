#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class LightingPass final : public Pass
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
                        .name           = "Position Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Albedo Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Scene TLAS",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::SceneData,
                    },
                    DependencyInfo {
                        .name           = "Lighting Result",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                },
            };
        }
    };
}
