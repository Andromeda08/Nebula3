#include "RenderGraphContext.hpp"

#include <format>
#include <fstream>
#include <print>

namespace rg
{
    RenderGraphContext::RenderGraphContext(const RenderGraphContextCreateInfo& createInfo)
    : mRHI(createInfo.rhi)
    {
        for (const auto nodeType : getAllNodeTypes())
        {
            if (mRHI->getFeatureLevel() >= getNodeRequiredFeatureLevel(nodeType))
            {
                mEnabledNodeTypes.insert(nodeType);
            }
        }

        // Setup import/export directory
        if (!std::filesystem::exists(sRenderGraphDirectory))
        {
            std::filesystem::create_directory(sRenderGraphDirectory);
        }
        // Load all Render Graphs from the directory
        int32_t loadedCount = 0;
        for (const auto& file : std::filesystem::directory_iterator(sRenderGraphDirectory))
        {
            if (file.is_regular_file())
            {
                if (auto loadResult = loadRenderGraph(file.path()); loadResult.has_value())
                {
                    mRenderGraphs.push_back(std::move(loadResult.value()));
                    loadedCount++;
                }
                else
                {
                    std::println("{}", loadResult.error());
                }
            }
        }

        if (loadedCount > 0)
        {
            std::println("[RenderGraph] Loaded {} saved RenderGraphs.", loadedCount);
        }
        else
        {
            // Create default empty graph if no render graphs were loaded.
            mRenderGraphs.push_back(RenderGraph::create({"Default Graph"}));
        }

        // Set first Render Graph as active.
        assert(!mRenderGraphs.empty());
        mActiveEditorGraph = mRenderGraphs[0].get();
    }

    const std::set<NodeType>& RenderGraphContext::getEnabledNodes() const noexcept
    {
        return mEnabledNodeTypes;
    }

    const std::vector<UPtr<RenderGraph>>& RenderGraphContext::getRenderGraphs() const
    {
        return mRenderGraphs;
    }

    void RenderGraphContext::setActiveEditorRenderGraph(RenderGraph* pRenderGraph)
    {
        mActiveEditorGraph = pRenderGraph;
    }

    RenderGraph* RenderGraphContext::getActiveEditorRenderGraph() const
    {
        return mActiveEditorGraph;
    }

    RenderGraph* RenderGraphContext::createRenderGraph()
    {
        const auto name = std::format("RenderGraph #{}", mRenderGraphs.size());
        mRenderGraphs.push_back(std::move(RenderGraph::create({ name })));
        mActiveEditorGraph = mRenderGraphs.back().get();
        return mActiveEditorGraph;
    }

    RenderGraphBuilder RenderGraphContext::createBuilder()
    {
        return RenderGraphBuilder(createRenderGraph());
    }

    RenderGraphCompilerResult RenderGraphContext::compileRenderGraph(RenderGraph* pRenderGraph)
    {
        const auto compiler = RenderGraphCompiler(pRenderGraph);
        const auto result = compiler.compile();

        std::println("[RenderGraph] Compilation of render graph {} : {}",
            pRenderGraph->getName(), result.success ? "Succeeded" : "Failed");

        for (const auto& message : result.messages)
        {
            std::println("{}", message);
        }

        return result;
    }

    std::expected<UPtr<RenderGraph>, std::string> RenderGraphContext::loadRenderGraph(const std::filesystem::path& filePath) const
    {
        const auto name = filePath.filename().string();
        std::ifstream ifs(filePath);
        if (!ifs.is_open())
        {
            return std::unexpected(std::format("[RenderGraph] Failed to load RenderGraph from file: {}", filePath.string()));
        }

        const nlohmann::json renderGraphJson = nlohmann::json::parse(ifs);
        if (renderGraphJson.at("version") != sRenderGraphSerializationVer)
        {
            return std::unexpected("[RenderGraph] Failed to load RenderGraph: out of date save format.");
        }

        if (auto deserializedRenderGraph = RenderGraph::deserializeRenderGraph(renderGraphJson, mEnabledNodeTypes);
            deserializedRenderGraph.has_value())
        {
            return std::move(deserializedRenderGraph.value());
        }
        else
        {
            return std::unexpected(std::format("[RenderGraph] Failed to parse RenderGraph\n\t- Reason: {}\n\t- File: {}",
                deserializedRenderGraph.error(), filePath.string()));
        }
    }

    void RenderGraphContext::saveRenderGraph(const RenderGraph* pRenderGraph)
    {
        auto modifiedName = pRenderGraph->getName();
        std::ranges::replace(modifiedName, ' ', '_');

        const auto filePath = std::format("{}/{}.json", sRenderGraphDirectory, modifiedName);
        std::ofstream ofs(filePath);
        if (!ofs.is_open())
        {
            std::println("[RenderGraph] Failed to save RenderGraph ({}): could not create file {}", pRenderGraph->getName(), filePath);
            return;
        }

        if (auto serialized = pRenderGraph->serializeRenderGraph(); serialized.has_value())
        {
            auto& json = serialized.value();
            json["version"] = sRenderGraphSerializationVer;

            ofs << json;
            std::println("[RenderGraph] Saved RenderGraph: {}", pRenderGraph->getName());
        }
        else
        {
            std::println("[RenderGraph] Error: {}", serialized.error());
        }
        ofs.close();
    }

    bool RenderGraphContext::hasQueuedRenderPathChange() const noexcept
    {
        return mNextRenderPath != nullptr;
    }

    void RenderGraphContext::queueRenderPath(const RenderGraphCompilerResult& compilerResult) noexcept
    {
        mNextRenderPath = RenderPath::create({
            .rhi            = mRHI,
            .compilerResult = compilerResult,
        });
    }

    void RenderGraphContext::changeToQueuedRenderPath() noexcept
    {
        if (!hasQueuedRenderPathChange())
        {
            std::println("[RenderGraph] There is no queued RenderPath to change to.");
            return;
        }
        mActiveRenderPath = std::move(mNextRenderPath);
        std::println("[RenderGraph] Changed to new RenderPath for graph: {}", mActiveRenderPath->getName());
    }

    RenderPath* RenderGraphContext::getCurrentRenderPath() const noexcept
    {
        return mActiveRenderPath.get();
    }
}
