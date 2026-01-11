#pragma once

#include "RenderGraph/RenderPath/Resource.hpp"
#include "Scene/Scene.hpp"

namespace rg
{
    class SceneResource final : public Resource
    {
    public:
        nbl_DISABLE_COPY(SceneResource);
        ~SceneResource() override = default;

        explicit SceneResource(Scene* pScene, const std::string& name)
        : Resource(name, ResourceType::SceneData)
        , mScene(pScene)
        {
        }

        [[nodiscard]] Scene* getScene() const noexcept
        {
            return mScene;
        }

    private:
        Scene* mScene;
    };
}
