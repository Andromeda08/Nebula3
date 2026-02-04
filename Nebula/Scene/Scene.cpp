#include "Scene.hpp"

#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mActiveCamera(nullptr)
, mName(createInfo.name)
{
    mTextureManager = TextureManager::create({
        .rhi = mRHI,
    });

    // Create camera uniform buffers
    for (auto& buffer : mCameraUniformBuffers)
    {
        buffer = mRHI->createBuffer({
            .size = sizeof(CameraData),
            .type = RHI::BufferType::Uniform,
            .label = "CameraUB",
        });
    }

    // Light system
    mLights = makeUnique<LightSystem>(mRHI);

    /* Scene Descriptor */
    {
        mSceneDescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding {
                    0, vk::DescriptorType::eUniformBuffer, 1,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
                },
                vk::DescriptorSetLayoutBinding {
                    1, vk::DescriptorType::eStorageBuffer, 1,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute
                },
            },
            .setCount = 2,
            .debugName = "SceneDescriptor",
        });

        for (auto i = 0; i < mSceneDescriptor->getSetCount(); i++)
        {
            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeUniformBuffer(0, mCameraUniformBuffers[i])
                .writeStorageBuffer(1, mLights->getDataBuffer());
            mSceneDescriptor->write(i, descriptorWrite);
        }
    }
}

void Scene::registerUIComponents(UserInterface* pUserInterface) const noexcept
{
}

void Scene::preFrame() noexcept
{
    mLights->upload();
}

void Scene::onEvent(const SDL_Event& event) const noexcept
{
    if (mActiveCamera)
    {
        mActiveCamera->onEvent(event);
    }
}

void Scene::onUpdate(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, float dt) noexcept
{
    if (mActiveCamera)
    {
        mActiveCamera->onUpdate();

        const auto cameraData = mActiveCamera->getCameraData();
        mCameraUniformBuffers[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
    }
}

void Scene::render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept
{
}

void Scene::addCamera(UPtr<ICamera> camera, const bool makeActive) noexcept
{
    mCameras.push_back(std::move(camera));
    if (makeActive || mCameras.size() == 1)
    {
        mActiveCamera = mCameras[0].get();
    }
}
