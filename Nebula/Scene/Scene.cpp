#include "Scene.hpp"

#include "Camera/OrbitCamera.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mName(createInfo.name)
{
    mTextureManager = TextureManager::create({
        .rhi = createInfo.rhi,
    });

    mCamera = OrbitCamera::create({
        .aspect = createInfo.rhi->getSwapchain()->getProperties().aspectRatio,
    });
    const auto cameraData = mCamera->getCameraData();
    for (auto& cameraUb : mCameraUB)
    {
        cameraUb = mRHI->createBuffer({
            .size      = sizeof(CameraData),
            .type      = RHI::BufferType::Uniform,
            .debugName = "CameraUB",
        });
        cameraUb->setData(&cameraData, sizeof(CameraData));
    }

    /* Scene Descriptor */ {
        mSceneDescriptor = mRHI->createDescriptor({
            .bindings     = {
                vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
            },
            .setCount     = 2,
            .debugName    = "SceneDescriptor",
        });

        for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
        {
            const auto bufferInfo = vk::DescriptorBufferInfo().setBuffer(mCameraUB[i]->getHandle()).setOffset(0).setRange(sizeof(CameraData));
            const auto descriptorWrite = RHI::DescriptorWriteInfo()
                .writeUniformBuffers(0, 1, &bufferInfo)
                .setSetIndex(i);
            mSceneDescriptor->write(descriptorWrite);
        }
    }
}

void Scene::registerUIComponents(UserInterface* pUserInterface) const
{
    pUserInterface->addComponent<OrbitCameraComponent>(
        dynamic_cast<OrbitCamera*>(mCamera.get()));
}

void Scene::update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt)
{
    const auto cameraData = mCamera->getCameraData();
    mCameraUB[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
}
