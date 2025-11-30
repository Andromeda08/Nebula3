#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Resource.hpp"
#include "Core/Macro.hpp"
#include "RenderGraph/Compiler/RenderGraphCompiler.hpp"
#include "RenderPass/IPass.hpp"
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
        RHI::Barrier                                    mInitialResourceBarriers;
        std::unordered_map<std::string, SPtr<Resource>> mResources;
        std::unordered_map<int32_t, RHI::Barrier>       mBarriers;
        std::vector<UPtr<IPass>>                        mPasses;
        std::vector<RenderPathLabel>                    mLabels;
        std::string                                     mRenderGraphName;
        bool                                            mIsInitialized = false;
    };
}
