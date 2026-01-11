#include "Scene.hpp"

#include "Camera/FlyingCamera.hpp"

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mName(createInfo.name)
, mPCSDF()
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

            // const auto descriptorWrite = RHI::DescriptorWriteInfo()
            //     .writeUniformBuffers(0, 1, mCameraUB[i]->getDescriptorInfo(sizeof(CameraData)))
            //     .setSetIndex(i);
            // mSceneDescriptor->write_old(descriptorWrite);
        }
    }

    /* Test Pipeline */ {
        mDepthBuffer = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eD32Sfloat,
            .usageFlags    = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            .createSampler = false,
            .debugName     = "DepthBuffer"
        });

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::DepthAttachment, mDepthBuffer });
             barrier.insert(commandList);
        });

        const auto colorAttachment = RHI::Attachment {
            .image = mRHI->getSwapchain()->getImage(0),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(0))
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        const auto depthAttachment = RHI::Attachment {
            .image = mDepthBuffer->getImage(),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setImageView(mDepthBuffer->getImageView())
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore),
        };
        mRenderPass = mRHI->createRenderPass({
            .renderArea       = mRHI->getSwapchain()->getProperties().area,
            .colorAttachments = { colorAttachment },
            .depthAttachment  = depthAttachment,
            .label            = "TestPipeline",
        });

        RHI::GraphicsPipelineCreateInfo pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange(vk::PushConstantRange().setOffset(0).setSize(sizeof(glm::vec4)).setStageFlags(vk::ShaderStageFlagBits::eFragment))
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](auto& stateInfo) {
                    stateInfo.attributeDescriptions = {{ 0, 0, vk::Format::eR32G32B32Sfloat, 0 }};
                    stateInfo.bindingDescriptions = {{ 0, sizeof(glm::vec3), vk::VertexInputRate::eVertex }};
                })
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/TestFwd.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/TestFwd.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("TestPipeline");

        mFwdPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    /* CIF SDF Compute Pass */ {
        mComputePrePass = makeUnique<viz::ComputePrePass>(mRHI, mCIFData->getAtomPositions());
    }

    mPCSDF.bboxMin = mComputePrePass->getBBoxMin();
    mPCSDF.bboxMax = mComputePrePass->getBBoxMax();
    mPCSDF.sesColor = glm::vec4(0.1, 0.38, 0.14, 1.0);
    mPCSDF.voxelSize = 0.5;
    mPCSDF.blending = 0.5;
    mPCSDF.ls = 1.0f;
    mPCSDF.useSubsurfaceScattering = 1;
    mPCSDF.rayMarchingSteps = 256;

    /* CIF Structure Rendering Pipeline */ {
        RHI::GraphicsPipelineCreateInfo pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange(vk::PushConstantRange().setOffset(0).setSize(sizeof(glm::vec4)).setStageFlags(vk::ShaderStageFlagBits::eFragment))
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](auto& stateInfo) {
                    stateInfo.attributeDescriptions = {
                        { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
                        { 1, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
                        { 2, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
                        { 3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
                        { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
                    };
                    stateInfo.bindingDescriptions = {
                        { 0, sizeof(glm::vec3), vk::VertexInputRate::eVertex },
                        { 1, sizeof(glm::mat4), vk::VertexInputRate::eInstance },
                    };
                })
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/Structure.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/Structure.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("StructurePipeline");

        mStructurePipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    /* SDF Descriptor Set */ {
        constexpr auto samplerCreateInfo = vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setAnisotropyEnable(true)
            .setMaxAnisotropy(1.0)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(false)
            .setCompareEnable(false)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMipLodBias(0.0f)
            .setMinLod(0.0f)
            .setMaxLod(0.0f);

        mSDFSampler = mRHI->getDevice()->getHandle().createSampler(samplerCreateInfo);

        mSDFDescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = 1,
            .debugName = "SDFDescriptor",
            });

        const auto imageInfo = vk::DescriptorImageInfo{ mSDFSampler, mComputePrePass->getSDFTexture3D()->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
        const auto write = RHI::DescriptorWriteInfo()
            .setSetIndex(0)
            .writeCombinedImageSamplers(0, 1, &imageInfo);
        mSDFDescriptor->write_old(write);
    }

    /* CIF SDF Rendering Pipeline */ {
        vk::PushConstantRange pcr;
        pcr.offset = 0;
        pcr.size = sizeof(PCSDF);
        pcr.stageFlags = vk::ShaderStageFlagBits::eFragment;

        const auto colorAttachment = RHI::Attachment {
            .image = mRHI->getSwapchain()->getImage(0),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(0))
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        mSDFRenderPass = mRHI->createRenderPass({
            .renderArea       = mRHI->getSwapchain()->getProperties().area,
            .colorAttachments = { colorAttachment },
            .label            = "SDFRenderPass",
        });

        RHI::GraphicsPipelineCreateInfo pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .addDescriptorSetLayout(mSDFDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addAttachmentState(RHI::PipelineUtils::makeColorBlendAttachmentState()
                    .setBlendEnable(true)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)))
            .addShader({ "Resources/Shaders/bin/FSQuad.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/SDF.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .setPushConstantRange(pcr)
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDebugName("SDFPipeline");

        mSDFPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }
}

void Scene::registerUIComponents(UserInterface* pUserInterface) const
{
}

void Scene::update(const RHI::CommandList* commandList, const RHI::FrameData& frameData, const float dt)
{
    const auto cameraData = mCamera->getCameraData();
    mCameraUB[frameData.currentFrame]->setData(&cameraData, sizeof(CameraData));
}

void Scene::render(const RHI::CommandList* commandList, const RHI::FrameData& frameData)
{
    auto sdfTexture = mComputePrePass->getSDFTexture3D();

    if (!mSRO.calculatedSDF || mSRO.recalculateSDF) {
        commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("SDF-Compute"));
        /* SDF to Storage Image usage */ {
            const auto barrier = RHI::Barrier()
                .addBarrier(sdfTexture->getBarrier(RHI::ImageUsage::StorageImage));
            barrier.insert(commandList);
        }

        // SDF Compute Pass
        mComputePrePass->execute(commandList);
        commandList->getHandle().endDebugUtilsLabelEXT();
        mSRO.calculatedSDF = true;
    }

    // Structure Pass
    if (mSRO.renderStructure) {
        commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("StructureRendering"));
        auto colorAttachment = RHI::Attachment{
            .image = mRHI->getSwapchain()->getImage(frameData.acquiredIndex),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(frameData.acquiredIndex))
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        mRenderPass->setColorAttachment(0, colorAttachment);
        mRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
            mStructurePipeline->bind(commandBuffer);
            mStructurePipeline->bindDescriptorSet(commandBuffer, mSceneDescriptor->getSet(frameData.currentFrame));
            mStructurePipeline->pushConstants(commandBuffer, &mStructureColor);

            static constexpr vk::DeviceSize offsets[2] = { 0, 0 };
            /* Spheres */ {
                const std::array vertexBuffers{ mCIFData->mSphereVertexBuffer->getHandle(), mCIFData->mSphereInstanceBuffer->getHandle() };
                commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
                commandBuffer.bindIndexBuffer(mCIFData->mSphereIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
                commandBuffer.drawIndexed(mCIFData->mCID.sphere.indices.size(), mCIFData->mCID.sphereTransforms.size(), 0, 0, 0);
            }
            /* Cylinders */ {
                const std::array vertexBuffers{ mCIFData->mCylinderVertexBuffer->getHandle(), mCIFData->mCylinderInstanceBuffer->getHandle() };
                commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
                commandBuffer.bindIndexBuffer(mCIFData->mCylinderIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
                commandBuffer.drawIndexed(mCIFData->mCID.cylinder.indices.size(), mCIFData->mCID.cylinderTransforms.size(), 0, 0, 0);
            }
            });
        commandList->getHandle().endDebugUtilsLabelEXT();
    }

    if (mSRO.renderSurface) {
        commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("SDF-Render"));
        /* SDF to (probably) ShaderReadOnly Image usage */ {
            const auto barrier = RHI::Barrier()
                .addBarrier(sdfTexture->getBarrier(RHI::ImageUsage::ShaderReadOnly));
            barrier.insert(commandList);
        }

        // SDF Render Pass
        // ❗Don't clear previous render pass result
        auto sdfColorAttachment = RHI::Attachment{
            .image = mRHI->getSwapchain()->getImage(frameData.acquiredIndex),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(frameData.acquiredIndex))
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        mSDFRenderPass->setColorAttachment(0, sdfColorAttachment);
        mSDFRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
            // TODO: SDF Render Pass commands
            mSDFPipeline->bind(commandBuffer);
            mSDFPipeline->pushConstants(commandBuffer, &mPCSDF);
            mSDFPipeline->bindDescriptorSets(commandBuffer, { mSceneDescriptor->getSet(frameData.currentFrame), mSDFDescriptor->getSet(0) });
            commandBuffer.draw(3, 1, 0, 0);
            });

        commandList->getHandle().endDebugUtilsLabelEXT();
    }
}
