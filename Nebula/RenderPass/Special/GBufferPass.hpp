#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class GBufferPass final : public Pass
    {
    public:
        ~GBufferPass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::GBufferPass,
                .displayName  = "G-Buffer Pass",
                .dependencies = {
                    DependencyInfo {
                        .name           = "Position Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Albedo Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Scene TLAS",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::SceneData,
                    },
                },
            };
        }
    };
}
