#pragma once

#include "RenderGraph/Node.hpp"
#include "RenderPass/Pass.hpp"

namespace rg
{
    class AmbientOcclusionPass final : public Pass
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
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::StorageImage },
                    },
                    DependencyInfo {
                        .name           = "Normal Buffer",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::StorageImage },
                    },
                    DependencyInfo {
                        .name           = "Scene TLAS",
                        .dependencyType = DependencyType::Read,
                        .resourceType   = ResourceType::SceneData,
                    },
                    DependencyInfo {
                        .name           = "AO Buffer",
                        .dependencyType = DependencyType::Write,
                        .resourceType   = ResourceType::Texture2D,
                        .resourceParams = ImageInfo { RHI::ImageUsage::StorageImage },
                    },
                },
            };
        }
    };
}
