#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"

namespace rg
{
    class GBufferPass final : public IPass
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
                        .name           = "Scene Data",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::SceneData,
                    },
                    DependencyInfo {
                        .name           = "Position Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Image,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Image,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                    DependencyInfo {
                        .name           = "Albedo Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Image,
                        .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
                    },
                },
            };
        }
    };
}
