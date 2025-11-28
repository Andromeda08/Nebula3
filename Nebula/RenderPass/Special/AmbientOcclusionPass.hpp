#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/IPass.hpp"

namespace rg
{
    class AmbientOcclusionPass final : public IPass
    {
    public:
        ~AmbientOcclusionPass() override = default;

        void execute(const RHI::CommandList* commandBuffer, const RHI::FrameData& frameData) override {}

        static NodeCreateInfo getNodeInfo()
        {
            return {
                .nodeType     = NodeType::AmbientOcclusionPass,
                .displayName  = "Ambient Occlusion",
                .subTitle     = "Mode: RTAO",
                .dependencies = {
                    DependencyInfo {
                        .name           = "Position Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::StorageImage,
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::StorageImage,
                    },
                    DependencyInfo {
                        .name           = "Top=level AS",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::TopLevelAS,
                    },
                    DependencyInfo {
                        .name           = "AO Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Image,
                        .imageUsage     = RHI::ImageUsage::StorageImage,
                    },
                },
            };
        }
    };
}
