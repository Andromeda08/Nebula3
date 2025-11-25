#pragma once

#include "RenderGraph/RenderGraphContext.hpp"
#include "UserInterface/IComponent.hpp"

namespace rg
{
    class RenderGraphEditorComponent final : public IComponent
    {
    public:
        explicit RenderGraphEditorComponent(const SPtr<RenderGraphContext>& renderGraphContext);

        ~RenderGraphEditorComponent() override = default;

        void draw() override;

        void update() override;

    private:
        void renderMenuBar();
        void renderNodeEditor() const;

        void handleCompile()    const;
        void handleConnection() const;
        void handleEdgeDelete() const;
        void handleNodeDelete() const;
        void handleResetGraph() const;
        void handleCreateGraph();

        RGConfiguration&         mConfiguration;
        RenderGraph*             mActiveGraph;
        SPtr<RenderGraphContext> mRenderGraphContext;
    };
}
