#pragma once

#include "Core/Types.hpp"
#include "RenderGraph/RGConfiguration.hpp"
#include "UserInterface/IComponent.hpp"

namespace rg
{
    class RenderGraph;
    class RenderGraphContext;

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
        void handleSaveGraph()  const;
        void handleCreateGraph();

        RGConfiguration&         mConfiguration;
        RenderGraph*             mActiveGraph;
        SPtr<RenderGraphContext> mRenderGraphContext;
        bool                     mFirstUpdate = true;
    };
}
