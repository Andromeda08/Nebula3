#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <set>
#include <vector>

#include "RenderGraph.hpp"
#include "RenderGraphTraits.hpp"
#include "RenderPath/RenderPath.hpp"

namespace rg
{
    struct RenderGraphContextCreateInfo
    {
        SPtr<RHI::VulkanRHI> rhi;
    };

    class RenderGraphContext
    {
    public:
        nbl_DISABLE_COPY(RenderGraphContext)
        nbl_CTOR_SHARED(RenderGraphContext);

        const std::set<NodeType>& getEnabledNodes() const noexcept;

        // =====================================
        // RenderGraph : Graph Management
        // =====================================

        const std::vector<UPtr<RenderGraph>>& getRenderGraphs() const;

        void setActiveEditorRenderGraph(RenderGraph* pRenderGraph);

        RenderGraph* getActiveEditorRenderGraph() const;

        RenderGraph* createRenderGraph();

        static RenderGraphCompilerResult compileRenderGraph(RenderGraph* pRenderGraph);

        // =====================================
        // RenderGraph : Serialization
        // =====================================

        [[nodiscard]] std::expected<UPtr<RenderGraph>, std::string> loadRenderGraph(const std::filesystem::path& filePath) const;

        static void saveRenderGraph(const RenderGraph* pRenderGraph);

        // =====================================
        // RenderGraph : GPU Realization
        // =====================================

        [[nodiscard]] bool hasQueuedRenderPathChange() const noexcept;

        void queueRenderPath(const RenderGraphCompilerResult& compilerResult) noexcept;

        void changeToQueuedRenderPath() noexcept;

        [[nodiscard]] RenderPath* getCurrentRenderPath() const noexcept;

    private:
        // Configuration
        constexpr static auto           sRenderGraphDirectory        = "Resources/RenderGraphs";
        constexpr static uint32_t       sRenderGraphSerializationVer = 2u;
        std::set<NodeType>              mEnabledNodeTypes;

        // Graph Management
        std::vector<UPtr<RenderGraph>>  mRenderGraphs;
        RenderGraph*                    mActiveEditorGraph = nullptr;

        // Graph GPU Realization
        UPtr<RenderPath>                mActiveRenderPath;
        UPtr<RenderPath>                mNextRenderPath;

        SPtr<RHI::VulkanRHI>            mRHI;
    };
}
