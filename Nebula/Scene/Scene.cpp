#include "Scene.hpp"

#include "Camera/FlyingCamera.hpp"
#include "Camera/OrbitCamera.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

Scene::Scene(const SceneCreateInfo& createInfo)
: mRHI(createInfo.rhi)
, mName(createInfo.name)
{
    // mTextureManager = TextureManager::create({
    //     .rhi = createInfo.rhi,
    // });

    /* CIF Loading */ {
        mCIF = makeUnique<CIFParser>("Resources/CIFFiles/IBP.cif");
        std::vector<glm::vec3> positions;
        for (const auto& [compId, v2] : mCIF->positions) {
            for (const auto& [atomId, v] : v2) {
                for (const auto& e : v) {
                    positions.push_back({ e.expected[0], e.expected[1], e.expected[2] });
                }
            }
        }
        /* Data Upload */ {
            const auto positionsSize = positions.size() * sizeof(glm::vec3);
            mMoleculePosBuffer = mRHI->createBuffer({
                .size      = positionsSize,
                .type      = RHI::BufferType::Storage,
                .debugName = "Molecule Positions",
            });

            const auto staging = mRHI->createBuffer({
                .size = positionsSize,
                .type = RHI::BufferType::Staging,
            });
            staging->setData(positions.data(), positionsSize);
            mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
                const auto copy = vk::BufferCopy2().setSrcOffset(0).setDstOffset(0).setSize(positionsSize);
                const auto info = vk::CopyBufferInfo2()
                    .setSrcBuffer(staging->getHandle())
                    .setDstBuffer(mMoleculePosBuffer->getHandle())
                    .setRegions(copy);
                commandList->getHandle().copyBuffer2(info);
            });
        }
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
            const auto descriptorWrite = RHI::DescriptorWrite()
                .addSetIndex(i)
                .writeUniformBuffer(0, { mCameraUB[i]->getHandle(), 0, sizeof(CameraData) });
            mSceneDescriptor->write(std::move(descriptorWrite));
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
                .addAttributeDescriptions<Vertex>()
                .addBindingDescriptions<Vertex>()
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/TestFwd.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/TestFwd.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthBuffer->getProperties().format)
            .setDebugName("TestPipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
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
    commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("Scene Render"));

    constexpr glm::vec4 color = { 0.75f, 0.1f, 0.5f, 1.0f };

    const auto colorAttachment = RHI::Attachment {
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
        mPipeline->bind(commandBuffer);
        mPipeline->bindDescriptorSet(commandBuffer, mSceneDescriptor->getSet(frameData.currentFrame));
        mPipeline->pushConstants(commandBuffer, &color);

        static constexpr vk::DeviceSize offsets[1] = { 0 };
        commandBuffer.bindVertexBuffers(0, 1, &mVertexBuffer->getHandle(), offsets);
        commandBuffer.bindIndexBuffer(mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(mCube->indexCount(), 1, 0, 0, 0);
    });

    commandList->getHandle().endDebugUtilsLabelEXT();
}
