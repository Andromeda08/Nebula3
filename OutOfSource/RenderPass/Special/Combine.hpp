#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class CombinePass final : public Pass
    {
    public:
        ~CombinePass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::CombinePass,
                .displayName  = "Combine",
                .subTitle     = "Mode: Add",
                .dependencies = {
                    DependencyInfo {
                        .name           = "A",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "B",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "Out",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                },
            };
        }
    };
}
