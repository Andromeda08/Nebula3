#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Resource.hpp"
#include "Core/Macro.hpp"
#include "RenderGraph/Compiler/RenderGraphCompiler.hpp"
#include "RenderPass/Pass.hpp"
#include "Resources/ResourceRegistry.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace rg
{
    struct RenderPathLabel
    {
        std::array<float, 3> color;
        std::string          name;
    };

    struct RenderPathCreateInfo
    {
        SPtr<RHI::VulkanRHI>        rhi;
        RenderGraphCompilerResult   compilerResult;
    };

    // =====================================
    // The GPU realization of a RenderGraph
    // =====================================
    class RenderPath
    {
    public:
        nbl_DISABLE_COPY(RenderPath);
        nbl_CTOR(RenderPath);

        void initialize(const RHI::CommandList* pCommandList);

        void update(float dt, const RHI::FrameData& frameData);

        void execute(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData);

        [[nodiscard]] const std::string& getName() const noexcept
        {
            return mRenderGraphName;
        }

    private:
        void createResources(const RenderPathCreateInfo& createInfo) noexcept;

        UPtr<ResourceRegistry> mResourceRegistry;

        RHI::Barrier                                    old_mInitialResourceBarriers;
        std::unordered_map<std::string, SPtr<Resource>> old_mResources;
        std::unordered_map<int32_t, RHI::Barrier>       old_mBarriers;
        std::vector<UPtr<Pass>>                         old_mPasses;
        std::vector<RenderPathLabel>                    old_mLabels;
        std::string                                     mRenderGraphName;
        bool                                            mIsInitialized = false;
    };
}
