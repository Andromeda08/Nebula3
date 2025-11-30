#include "RenderGraphEditorComponent.hpp"

#include <print>
#include <imnodes.h>
#include <imgui_stdlib.h>

#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/RenderGraphContext.hpp"

namespace rg
{
    RenderGraphEditorComponent::RenderGraphEditorComponent(const SPtr<RenderGraphContext>& renderGraphContext)
    : mConfiguration(Configuration::getConfig().renderGraph)
    , mRenderGraphContext(renderGraphContext)
    {
        mActiveGraph = mRenderGraphContext->getActiveEditorRenderGraph();
    }

    void RenderGraphEditorComponent::draw()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 16.0f, 16.0f });
        ImGui::Begin("RenderGraph Editor", nullptr, ImGuiWindowFlags_MenuBar);
        {
            renderMenuBar();
            renderNodeEditor();
            handleConnection();
            handleEdgeDelete();
            handleNodeDelete();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void RenderGraphEditorComponent::update()
    {
        if (mFirstUpdate)
        {
            mFirstUpdate = false;
            return;
        }

        // Update Node saved grid positions
        for (auto& node : mActiveGraph->getNodes())
        {
            const auto gridPos = ImNodes::GetNodeGridSpacePos(node->getId());
            node->setGridPos(gridPos);
        }
    }

    void RenderGraphEditorComponent::renderMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Add Node"))
            {
                for (const auto& nodeType : getAllNodeTypes())
                {
                    if (ImGui::MenuItem(
                        toString(nodeType).c_str(), nullptr, false,
                        mRenderGraphContext->getEnabledNodes().contains(nodeType)))
                    {
                        if (isSourceNode(nodeType) && mActiveGraph->hasSourceNode())
                        {
                            std::println("[RenderGraph] The RenderGraph can only have one source node.");
                            continue;
                        }
                        if (isSinkNode(nodeType) && mActiveGraph->hasSinkNode())
                        {
                            std::println("[RenderGraph] The RenderGraph can only have one sink node.");
                            continue;
                        }

                        try {
                            const auto nodeInfo = getNodeCreateInfo(nodeType);
                            mActiveGraph->addNode(nodeInfo);
                        } catch (const std::runtime_error& ex) {
                            std::println("[RenderGraph] {}", ex.what());
                        }
                    }
                }

                ImGui::EndMenu();
            }

            if (!mActiveGraph->getSupportsCurrentPlatform()) { ImGui::BeginDisabled(); }
            if (ImGui::Button("Compile"))
            {
                handleCompile();
            }
            if (!mActiveGraph->getSupportsCurrentPlatform()) { ImGui::EndDisabled(); }

            if (ImGui::Button("Save"))
            {
                handleSaveGraph();
            }

            if (ImGui::Button("Reset"))
            {
                handleResetGraph();
            }

            if (ImGui::BeginMenu("Graphs"))
            {
                if (ImGui::MenuItem("New Graph", nullptr, false, true))
                {
                    handleCreateGraph();
                }

                ImGui::Separator();

                for (const auto& graph : mRenderGraphContext->getRenderGraphs())
                {
                    if (ImGui::MenuItem(graph->getName().c_str(), nullptr, false, mActiveGraph->getName() != graph->getName()))
                    {
                        mRenderGraphContext->setActiveEditorRenderGraph(graph.get());
                        mActiveGraph = mRenderGraphContext->getActiveEditorRenderGraph();
                        std::println("[RenderGraph] Changed to RenderGraph: {}", mActiveGraph->getName());
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Text("[Current Graph: %s]", mActiveGraph->getName().c_str());

            ImGui::EndMenuBar();
        }
    }

    void RenderGraphEditorComponent::renderNodeEditor() const
    {
        ImNodes::BeginNodeEditor();
        {
            if (!mActiveGraph->getSupportsCurrentPlatform())
            {
                ImGui::TextColored(ImVec4(251, 44, 54, 255), "This graph is only editable and cannot be compiled.");
            }

            ImGui::Dummy(ImVec2(0, 16));
            ImGui::Indent(16);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::BeginChild("graphEdit", ImVec2(256, 36), true);
            {
                ImGui::SetNextItemWidth(200);
                ImGui::InputText("Name", mActiveGraph->getPName());
            }
            ImGui::PopStyleVar();
            ImGui::Unindent(16);
            ImGui::EndChild();

            ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 4.0f);
            ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 3.0f);
            for (const auto& node : mActiveGraph->getNodes())
            {
                node->draw();
            }
            for (const auto& edge : mActiveGraph->getEdges())
            {
                const auto& linkColor = mConfiguration.getResourceStyle(edge.resourceType).cLink;
                ImNodes::PushColorStyle(ImNodesCol_Link, nbl_TO_IM_COL32(linkColor));
                ImNodes::Link(edge.id, edge.srcDependencyId, edge.dstDependencyId);
                ImNodes::PopColorStyle();
            }
        }
        ImNodes::EndNodeEditor();
    }

    void RenderGraphEditorComponent::handleCompile() const
    {
        const auto result = RenderGraphContext::compileRenderGraph(mActiveGraph);
        mRenderGraphContext->queueRenderPath(result);
    }

    void RenderGraphEditorComponent::handleConnection() const
    {
        int32_t startNode, startAttr, endNode, endAttr;
        if (ImNodes::IsLinkCreated(&startNode, &startAttr, &endNode, &endAttr))
        {
            try
            {
                // Throws an exception when it's invalid, this is to test what's the correct start / end point.
                const auto& attr = mActiveGraph->getNode(startNode)->getDependencyInfo(startAttr);
            }
            catch (const std::runtime_error& ex)
            {
                std::swap(startNode, endNode);
                std::swap(startAttr, endAttr);
            }

            mActiveGraph->addEdge(startNode, startAttr, endNode, endAttr);
        }
    }

    void RenderGraphEditorComponent::handleEdgeDelete() const
    {
        if (const int32_t nSelected = ImNodes::NumSelectedLinks();
            nSelected > 0 && ImGui::IsKeyReleased(mConfiguration.bindDeleteEdge))
        {
            std::vector<int32_t> selectedLinks;
            selectedLinks.resize(static_cast<size_t>(nSelected));
            ImNodes::GetSelectedLinks(selectedLinks.data());
            for (const int32_t edgeId : selectedLinks)
            {
                mActiveGraph->eraseEdge(edgeId);
            }
        }
    }

    void RenderGraphEditorComponent::handleNodeDelete() const
    {
        if (const int32_t nSelected = ImNodes::NumSelectedNodes();
            nSelected > 0 && ImGui::IsKeyReleased(mConfiguration.bindDeleteNode))
        {
            std::vector<int32_t> selectedNodes;
            selectedNodes.resize(static_cast<size_t>(nSelected));
            ImNodes::GetSelectedNodes(selectedNodes.data());
            for (const int32_t nodeId : selectedNodes)
            {
                mActiveGraph->eraseNode(nodeId);
            }
        }
    }

    void RenderGraphEditorComponent::handleResetGraph() const
    {
        mActiveGraph->reset();
        std::println("[RenderGraph] The current RenderGraph has been reset.");
    }

    void RenderGraphEditorComponent::handleSaveGraph() const
    {
        mRenderGraphContext->saveRenderGraph(mActiveGraph);
    }

    void RenderGraphEditorComponent::handleCreateGraph()
    {
        mActiveGraph = mRenderGraphContext->createRenderGraph();
        std::println("[RenderGraph] Created new empty RenderGraph");
    }
}
