#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class AntiAliasingPass final : public Pass
    {
    public:
        ~AntiAliasingPass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::AntiAliasingPass,
                .displayName  = "Anti-Aliasing",
                .subTitle     = "Mode: FXAA",
                .dependencies = {
                    DependencyInfo {
                        .name           = "AA Input",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ShaderReadOnly },
                    },
                    DependencyInfo {
                        .name           = "AA Output",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                },
            };
        }
    };
}
