#pragma once

#include "GPUVoxelInstanceData.hpp"
#include "TerrainGenerator.hpp"
#include "Math/Transform.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Camera/FlyingCamera.hpp"
#include "Scene/Geometry/Geometry.hpp"
#include "VulkanRHI/Barrier.hpp"

struct VoxelSceneParams
{
    glm::mat4 globalScale = Transform().setScale(glm::vec3(0.1f)).getModel();
};

class VoxelScene : public Scene
{
public:
    explicit VoxelScene(const SceneCreateInfo& createInfo)
    : Scene(createInfo)
    {
        mCube = addGeometry<Cube>(Cube::Params{});

        const auto vertexSize = mCube->vertexCount() * sizeof(Vertex);
        mVertexBuffer = mRHI->createBuffer({
            .size  = vertexSize,
            .type  = RHI::BufferType::Vertex,
            .label = "Voxel-VertexBuffer",
        });
        mRHI->immediate_uploadToBuffer(mVertexBuffer.get(), mCube->getVertices().data(), vertexSize);

        const auto indexSize = mCube->indexCount() * sizeof(uint32_t);
        mIndexBuffer = mRHI->createBuffer({
            .size  = indexSize,
            .type  = RHI::BufferType::Index,
            .label = "Voxel-IndexBuffer",
        });
        mRHI->immediate_uploadToBuffer(mIndexBuffer.get(), mCube->getIndices().data(), indexSize);

        const auto [width, height] = mRHI->getSwapchain()->getProperties().extent;
        auto camera = makeUnique<FlyingCamera>(glm::ivec2(width, height), glm::vec3(0.0f, 0.0f, 5.0f));
        addCamera(std::move(camera), true);

        generateTerrainVoxelData();

        createForwardPass();
    }

    void render(const RHI::CommandList* commandList, const RHI::FrameData& frameData) noexcept override
    {
        commandList->beginLabel("VoxelScene");

        mRenderPass->setColorAttachment(0, RHI::Attachment{
            .image = mRHI->getSwapchain()->getImage(frameData.acquiredIndex),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(frameData.acquiredIndex))
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        });

        mRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
            mPipeline->bind(commandBuffer);
            mPipeline->bindDescriptorSet(commandBuffer, mSceneDescriptor->getSet(frameData.currentFrame));
            mPipeline->pushConstants(commandBuffer, &mParams);

            static constexpr vk::DeviceSize offsets[2] = { 0, 0 };
            const std::array vertexBuffers{ mVertexBuffer->getHandle(), mInstanceBuffer->getHandle() };
            commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
            commandBuffer.bindIndexBuffer(mIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(mCube->indexCount(), mInstanceData.size(), 0, 0, 0);
        });

        commandList->endLabel();
    }

private:
    void generateTerrainVoxelData() noexcept
    {
        auto terrainGenerator = vxl::TerrainGenerator({ 256, 24, 128, true });

        terrainGenerator.generate();

        mInstanceData = terrainGenerator.getResult()
            | std::views::transform([](const vxl::VoxelData& data) -> GPUVoxelInstanceData {
                auto t = Transform().setTranslate(data.position);
                return {
                    .model = t.getModel(),
                    .color = glm::vec4(data.color, 1.0f),
                };
            })
            | std::ranges::to<std::vector>();
        spdlog::debug("Generated {} voxels", mInstanceData.size());

        const auto size = mInstanceData.size() * sizeof(GPUVoxelInstanceData);
        mInstanceBuffer = mRHI->createBuffer({
            .size  = size,
            .type  = RHI::BufferType::Vertex,
            .label = "Voxel-InstanceData",
        });
        mRHI->immediate_uploadToBuffer(mInstanceBuffer.get(), mInstanceData.data(), size);
    }

    void createForwardPass() noexcept
    {
        mDepthImage = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eD32Sfloat,
            .usageFlags    = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            .createSampler = false,
            .debugName     = "VoxelForward-DepthImage"
        });

        mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* commandList) -> void {
            const auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::DepthAttachment, mDepthImage });
             barrier.insert(commandList);
        });

        mRenderPass = mRHI->createRenderPass({
            .renderArea = mRHI->getSwapchain()->getProperties().area,
            .colorAttachments = {
                RHI::Attachment {
                    .image = mRHI->getSwapchain()->getImage(0),
                    .attachmentInfo = vk::RenderingAttachmentInfo()
                        .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}))
                        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                        .setImageView(mRHI->getSwapchain()->getImageView(0))
                        .setLoadOp(vk::AttachmentLoadOp::eClear)
                        .setStoreOp(vk::AttachmentStoreOp::eStore)
                }
            },
            .depthAttachment  = RHI::Attachment {
                .image = mDepthImage->getImage(),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                   .setClearValue(vk::ClearValue().setDepthStencil({1.0f, 0}))
                   .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                   .setImageView(mDepthImage->getImageView())
                   .setLoadOp(vk::AttachmentLoadOp::eClear)
                   .setStoreOp(vk::AttachmentStoreOp::eStore),
            },
            .label = "VoxelForward-Pass",
        });

        RHI::GraphicsPipelineCreateInfo pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(VoxelSceneParams) })
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](auto& stateInfo) {
                    stateInfo.attributeDescriptions = {
                        { 0, 0, vk::Format::eR32G32B32Sfloat,    sizeof(glm::vec3) * 0 },
                        { 1, 0, vk::Format::eR32G32B32Sfloat,    sizeof(glm::vec3) * 1 },
                        { 2, 0, vk::Format::eR32G32Sfloat,       sizeof(glm::vec3) * 2 },
                        { 3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
                        { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
                        { 5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
                        { 6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
                        { 7, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 4 },
                    };
                    stateInfo.bindingDescriptions = {
                        { 0, sizeof(Vertex),               vk::VertexInputRate::eVertex },
                        { 1, sizeof(GPUVoxelInstanceData), vk::VertexInputRate::eInstance },
                    };
                })
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/VoxelForward.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/VoxelForward.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthImage->getProperties().format)
            .setDebugName("VoxelForward-Pipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    Geometry*                                   mCube;
    SPtr<RHI::Buffer>                           mVertexBuffer;
    SPtr<RHI::Buffer>                           mIndexBuffer;

    std::vector<GPUVoxelInstanceData>           mInstanceData;
    SPtr<RHI::Buffer>                           mInstanceBuffer;

    VoxelSceneParams                            mParams;

    // Forward Pass
    SPtr<RHI::Image>                            mDepthImage;
    SPtr<RHI::GraphicsPipeline>                 mPipeline;
    SPtr<RHI::RenderPass>                       mRenderPass;
};
