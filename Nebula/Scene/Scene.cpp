#include "Scene.hpp"

#include "Camera/OrbitCamera.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mName(createInfo.name)
{
    mTextureManager = TextureManager::create({
        .rhi = createInfo.rhi,
    });

    mCamera = OrbitCamera::create({
        .aspect = createInfo.rhi->getSwapchain()->getProperties().aspectRatio,
    });
}

void Scene::registerUIComponents(UserInterface* pUserInterface) const
{
    pUserInterface->addComponent<OrbitCameraComponent>(
        dynamic_cast<OrbitCamera*>(mCamera.get()));
}
