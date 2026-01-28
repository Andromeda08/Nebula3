#include "Scene.hpp"

#include "Camera/FlyingCamera.hpp"
#include "Components/MoleculeRenderingUI.hpp"
#include "RenderPass/Molecule/StructurePass.hpp"

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mName(createInfo.name)
{
    mTextureManager = TextureManager::create({
        .rhi = mRHI,
    });

    /* CIF Loading */ {
        mCIFData = makeUnique<CIFData>(CIFDataCreateInfo{ Configuration::getMoleculeFile(), true, mRHI });
    }

    /* (Flying) Camera */ {
        const auto e = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(e.width, e.height), glm::vec3(0.0f, 0.0f, 5.0f));
        const auto cameraData = mCamera->getCameraData();
        for (auto& cameraUb : mCameraUB)
        {
            cameraUb = mRHI->createBuffer({
                .size  = sizeof(CameraData),
                .type  = RHI::BufferType::Uniform,
                .label = "CameraUB",
            });
            cameraUb->setData(&cameraData, sizeof(CameraData));
        }
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
            const auto descriptorWrite = RHI::DescriptorWrite()
                .writeUniformBuffer(0, mCameraUB[i]);
            mSceneDescriptor->write(i, descriptorWrite);
        }
    }

    /* Molecule Rendering : Renderpasses */ {
        mSDFComputePass  = makeUnique<Molecule::SDFComputePass>(mRHI, mCIFData->getAtomPositions());
        mStructurePass   = makeUnique<Molecule::StructurePass>(mRHI, mSceneDescriptor, mCIFData.get());
        mSDFRaymarchPass = makeUnique<Molecule::SDFRaymarchPass>(mRHI, mSceneDescriptor, mSDFComputePass->getSDFTexture3D());

        mSDFRaymarchPass->setParams({
            .bboxMin = mSDFComputePass->getPushConstants().bboxMin,
            .bboxMax = mSDFComputePass->getPushConstants().bboxMax,
            .sesColor = glm::vec4(0.1f, 0.38f, 0.14f, 1.0f),
            .voxelSize = 0.5f,
            .blending = 0.5f,
            .ls = 1.0f,
            .useSubsurfaceScattering = 1,
            .rayMarchingSteps = 256
        });
    }
}

void Scene::registerUIComponents(UserInterface* pUserInterface) const
{
    pUserInterface->addComponent<MoleculeRenderingUI>(&mMoleculeRenderingOptions, mSDFComputePass.get(), mStructurePass.get(), mSDFRaymarchPass.get());
}

void Scene::update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt)
{
    mCamera->onUpdate();

    const auto cameraData = mCamera->getCameraData();
    mCameraUB[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
}

void Scene::render(const RHI::CommandList* commandList, const RHI::FrameData& frameData)
{
    if (!mMoleculeRenderingOptions.hasCalculatedSDF || mMoleculeRenderingOptions.shouldRecalculateSDF)
    {
        mSDFComputePass->execute(commandList, frameData);
        mMoleculeRenderingOptions.hasCalculatedSDF = true;
    }

    if (mMoleculeRenderingOptions.renderStructure)
    {
        mStructurePass->execute(commandList, frameData);
    }

    if (mMoleculeRenderingOptions.renderSurface)
    {
        mSDFRaymarchPass->execute(commandList, frameData);
    }
}
