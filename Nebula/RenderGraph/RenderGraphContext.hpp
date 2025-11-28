#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <set>
#include <vector>

#include "RenderGraph.hpp"
#include "RenderGraphTraits.hpp"

namespace rg
{
    struct RenderGraphContextCreateInfo
    {
        RHIFeatureLevel rhiFeatureLevel;
    };

    class RenderGraphContext
    {
    public:
        nbl_DISABLE_COPY(RenderGraphContext)
        nbl_CTOR_SHARED(RenderGraphContext);

        const std::set<NodeType>& getEnabledNodes() const noexcept;

        const std::vector<UPtr<RenderGraph>>& getRenderGraphs() const;

        void setActiveEditorRenderGraph(RenderGraph* pRenderGraph);

        RenderGraph* getActiveEditorRenderGraph() const;

        RenderGraph* createRenderGraph();

        [[nodiscard]] std::expected<UPtr<RenderGraph>, std::string> loadRenderGraph(const std::filesystem::path& filePath);

        static void saveRenderGraph(const RenderGraph* pRenderGraph);

    private:
        constexpr static auto sRenderGraphDirectory = "Resources/RenderGraphs";

        // Configuration
        std::set<NodeType>              mEnabledNodeTypes;

        // RenderGraphs
        std::vector<UPtr<RenderGraph>>  mRenderGraphs;
        RenderGraph*                    mActiveEditorGraph = nullptr;

        // RenderPath
    };
}
