#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <set>
#include <vector>

#include "RenderGraph.hpp"
#include "RenderGraphTraits.hpp"
#include "Builder/RenderGraphBuilder.hpp"
#include "RenderPath/RenderPath.hpp"
#include "Scene/Scene.hpp"

namespace rg
{
    struct RenderGraphContextCreateInfo
    {
        SPtr<RHI::VulkanRHI> rhi;
    };

    class RenderGraphContext
    {
    public:
        // Constants
        constexpr static auto     sRenderGraphDirectory        = "Resources/RenderGraphs";
        constexpr static auto     sRenderGraphExportDirectory  = "Resources/RenderGraphs/Export";
        constexpr static uint32_t sRenderGraphSerializationVer = 2u;

        nbl_DISABLE_COPY(RenderGraphContext)
        nbl_CTOR_SHARED(RenderGraphContext);

        const std::set<NodeType>& getEnabledNodes() const noexcept;

        [[nodiscard]] Scene* getActiveScene() const noexcept
        {
            return mActiveScene;
        }

        // =====================================
        // RenderGraph : Graph Management
        // =====================================

        const std::vector<UPtr<RenderGraph>>& getRenderGraphs() const;

        void setActiveEditorRenderGraph(RenderGraph* pRenderGraph);

        RenderGraph* getActiveEditorRenderGraph() const;

        /**
         * Create a new, empty RenderGraph
         * @param changeActiveGraph If true, the active (editor) RenderGraph is changed for the new one.
         * @return ref to the new RenderGraph
         */
        RenderGraph* createRenderGraph(bool changeActiveGraph = false);

        /**
         * Spawn a RenderGraph builder with a fresh, empty RenderGraph.
         * @return RenderGraphBuilder
         */
        RenderGraphBuilder createBuilder();

        /**
         * Compile the specified RenderGraph
         * @param pRenderGraph
         */
        static RenderGraphCompilerResult compileRenderGraph(RenderGraph* pRenderGraph);

        // =====================================
        // RenderGraph : Serialization
        // =====================================

        /**
         * Load a RenderGraph from the specified file.
         * @param filePath Path to JSON file containing a saved RenderGraph
         * @return UPtr to the loaded RenderGraph or an error message
         */
        [[nodiscard]] std::expected<UPtr<RenderGraph>, std::string> loadRenderGraph(const std::filesystem::path& filePath) const;

        /**
         * Save the specified RenderGraph to a JSON file.
         * @param pRenderGraph
         */
        static void saveRenderGraph(const RenderGraph* pRenderGraph);

        // =====================================
        // RenderGraph : GPU Realization
        // =====================================

        [[nodiscard]] bool hasQueuedRenderPathChange() const noexcept;

        void queueRenderPath(const RenderGraphCompilerResult& compilerResult) noexcept;

        void changeToQueuedRenderPath() noexcept;

        [[nodiscard]] RenderPath* getCurrentRenderPath() const noexcept;

    private:
        void createInitialRenderPath();

        Scene*                          mActiveScene = nullptr;

        // Configuration
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
