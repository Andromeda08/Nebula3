#include "Scene.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mName(createInfo.name)
{
    mTextureManager = TextureManager::create({
        .rhi = createInfo.rhi,
    });
}
