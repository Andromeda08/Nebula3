#include "RenderGraphEditorComponent.hpp"

#include <print>
#include <imnodes.h>

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
                        if (nodeType == NodeType::Scene && mActiveGraph->hasSourceNode())
                        {
                            std::println("[RenderGraph] The RenderGraph can only have one Scene Node.");
                            continue;
                        }
                        if (nodeType == NodeType::Present && mActiveGraph->hasSinkNode())
                        {
                            std::println("[RenderGraph] The RenderGraph can only have one Present Node.");
                            continue;
                        }

                        mActiveGraph->addNode({
                            .nodeType     = nodeType,
                            .displayName  = toString(nodeType),
                            .dependencies = {
                                DependencyInfo { RenderGraph::nextId(), "Read", DependencyType::Read, ResourceType::Image, false, mConfiguration.getResourceStyle(ResourceType::Image) },
                                DependencyInfo { RenderGraph::nextId(), "Write", DependencyType::Write, ResourceType::Image, false, mConfiguration.getResourceStyle(ResourceType::Image) },
                            },
                            .nodeStyle    = mConfiguration.getNodeStyle(nodeType),
                        });
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::Button("Compile"))
            {
                handleCompile();
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

                        // Restore node positions
                        for (auto& node : mActiveGraph->getNodes())
                        {
                            ImNodes::SetNodeGridSpacePos(node->getId(), node->getGridPos());
                        }

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
        std::println("[RenderGraph] Compilation not implemented yet!");
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

    void RenderGraphEditorComponent::handleCreateGraph()
    {
        mActiveGraph = mRenderGraphContext->createRenderGraph();
        std::println("[RenderGraph] Created new empty RenderGraph");
    }
}
