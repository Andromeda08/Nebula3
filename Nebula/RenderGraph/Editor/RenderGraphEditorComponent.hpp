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

    private:
        SPtr<RenderGraphContext> mRenderGraphContext;
    };
}
