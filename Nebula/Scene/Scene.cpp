#include "Scene.hpp"

#include "Camera/FlyingCamera.hpp"

#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mName(createInfo.name)
{
    /* CIF Loading */ {
        mCIFData = makeUnique<CIFData>(CIFDataCreateInfo{ "Resources/CIFFiles/IBP.cif", true, mRHI });
    }

    /* Cube Object & Vertex/Index Buffers */ {
        mCube = makeShared<Cube>(Cube::Params { 0.5f });

        const auto vertexSize = mCube->vertexCount() * sizeof(Vertex);
        mVertexBuffer = mRHI->createBuffer({
            .size      = vertexSize,
            .type      = RHI::BufferType::Vertex,
            .debugName = "Cube-VertexBuffer",
        });

        const auto indexSize = mCube->indexCount() * sizeof(uint32_t);
        mIndexBuffer = mRHI->createBuffer({
            .size      = indexSize,
            .type      = RHI::BufferType::Index,
            .debugName = "Cube-IndexBuffer",
        });

        const auto stagingBuffer = mRHI->createBuffer({
            .size      = vertexSize + indexSize,
            .type      = RHI::BufferType::Staging,
            .debugName = "Cube-Staging",
        });
        stagingBuffer->setData(mCube->getVertices().data(), vertexSize, 0);
        stagingBuffer->setData(mCube->getIndices().data(), indexSize, vertexSize);

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            const auto vertexRegion = vk::BufferCopy2().setSrcOffset(0).setDstOffset(0).setSize(vertexSize);
            const auto vertexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mVertexBuffer->getHandle())
                .setRegions(vertexRegion);
            commandList->getHandle().copyBuffer2(vertexCopy);

            const auto indexRegion = vk::BufferCopy2().setSrcOffset(vertexSize).setDstOffset(0).setSize(indexSize);
            const auto indexCopy = vk::CopyBufferInfo2()
                .setSrcBuffer(stagingBuffer->getHandle())
                .setDstBuffer(mIndexBuffer->getHandle())
                .setRegions(indexRegion);
            commandList->getHandle().copyBuffer2(indexCopy);
        });
    }

    /* (Flying) Camera */ {
        const auto e = mRHI->getSwapchain()->getProperties().extent;
        mCamera = makeUnique<FlyingCamera>(glm::ivec2(e.width, e.height), glm::vec3(0.0f, 0.0f, 5.0f));
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
            mSceneDescriptor->write_old(descriptorWrite);
        }
    }

    /* Test Pipeline */ {
        mDepthBuffer = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eD32Sfloat,
            .usageFlags    = vk::ImageUsageFlagBits::eDepthStencilAttachment,
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
                // .addAttributeDescriptions<Vertex>()
                // .addBindingDescriptions<Vertex>()
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/TestFwd.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/TestFwd.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("TestPipeline");

        mFwdPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    /* CIF SDF Compute Pass */ {
        mComputePrePass = makeUnique<viz::ComputePrePass>(mRHI, mCIFData->mAtomPositionsBuffer);
    }

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

    commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("SDF-Compute"));
    /* SDF to Storage Image usage */ {
        const auto barrier = RHI::Barrier()
            .addBarrier(sdfTexture->getBarrier(RHI::ImageUsage::StorageImage));
        barrier.insert(commandList);
    }

    // SDF Compute Pass
    mComputePrePass->execute(commandList);
    commandList->getHandle().endDebugUtilsLabelEXT();

    // Structure Pass
    commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("StructureRendering"));
     auto colorAttachment = RHI::Attachment {
        .image = mRHI->getSwapchain()->getImage(0),
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
            const std::array vertexBuffers { mCIFData->mSphereVertexBuffer->getHandle(), mCIFData->mSphereInstanceBuffer->getHandle() };
            commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
            commandBuffer.bindIndexBuffer(mCIFData->mSphereIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(mCIFData->mCID.sphere.indices.size(), mCIFData->mCID.sphereTransforms.size(), 0, 0, 0);
        }
        /* Cylinders */ {
            const std::array vertexBuffers { mCIFData->mCylinderVertexBuffer->getHandle(), mCIFData->mCylinderInstanceBuffer->getHandle() };
            commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
            commandBuffer.bindIndexBuffer(mCIFData->mCylinderIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(mCIFData->mCID.cylinder.indices.size(), mCIFData->mCID.cylinderTransforms.size(), 0, 0, 0);
        }
    });
    commandList->getHandle().endDebugUtilsLabelEXT();

    commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("SDF-Render"));
    /* SDF to (probably) ShaderReadOnly Image usage */ {
        const auto barrier = RHI::Barrier()
            .addBarrier(sdfTexture->getBarrier(RHI::ImageUsage::ShaderReadOnly));
        barrier.insert(commandList);
    }

    // SDF Render Pass
    // ❗Don't clear previous render pass result
    colorAttachment.attachmentInfo.setLoadOp(vk::AttachmentLoadOp::eLoad);
    mRenderPass->setColorAttachment(0, colorAttachment);
    // mRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
        // TODO: SDF Render Pass commands
    // });

    commandList->getHandle().endDebugUtilsLabelEXT();
}
