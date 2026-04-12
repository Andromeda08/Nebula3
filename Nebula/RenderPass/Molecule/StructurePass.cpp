#include "StructurePass.hpp"

#include "Scene/Types/Vertex.hpp"
#include "VulkanRHI/Barrier.hpp"

namespace Molecule
{
    StructurePass::StructurePass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Descriptor>& sceneDescriptor, CIFData* pCIFData)
    : Pass()
    , mCIFData(pCIFData)
    , mRHI(rhi)
    , mSceneDescriptor(sceneDescriptor)
    {
        mDepthImage = mRHI->createImage({
            .extent        = mRHI->getSwapchain()->getProperties().extent,
            .format        = vk::Format::eD32Sfloat,
            .usageFlags    = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            .createSampler = false,
            .debugName     = "StructurePass-DepthImage"
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
            .label = "TestPipeline",
        });

        RHI::GraphicsPipelineCreateInfo pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(StructurePassParams) })
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .configure([](auto& stateInfo) {
                    stateInfo.attributeDescriptions = {
                        { 0, 0, vk::Format::eR32G32B32Sfloat,    sizeof(glm::vec3) * 0 },
                        { 1, 0, vk::Format::eR32G32B32Sfloat,    sizeof(glm::vec3) * 1 },
                        { 2, 0, vk::Format::eR32G32B32Sfloat,    sizeof(glm::vec3) * 2 },
                        { 3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 0 },
                        { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 1 },
                        { 5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2 },
                        { 6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3 },
                        { 7, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 4 },
                        { 8, 1, vk::Format::eR32Sint,            sizeof(glm::vec4) * 5 },
                    };
                    stateInfo.bindingDescriptions = {
                        { 0, sizeof(Vertex),                vk::VertexInputRate::eVertex },
                        { 1, sizeof(GPUObjectInstanceData), vk::VertexInputRate::eInstance },
                    };
                })
                .addAttachmentState())
            .addShader({ "Resources/Shaders/bin/Structure.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/Structure.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDepthAttachmentFormat(mDepthImage->getProperties().format)
            .setDebugName("StructurePipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
    }

    void StructurePass::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData)
    {
        commandList->beginLabel("Molecule::StructurePass");

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
            /* Spheres */ {
                const std::array vertexBuffers{ mCIFData->mSphereVertexBuffer->getHandle(), mCIFData->mSphereInstanceBuffer->getHandle() };
                commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
                commandBuffer.bindIndexBuffer(mCIFData->mSphereIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
                commandBuffer.drawIndexed(mCIFData->mCID.sphere->getIndexCount(), mCIFData->mCID.sphereTransforms.size(), 0, 0, 0);
            }
            /* Cylinders */ {
                const std::array vertexBuffers{ mCIFData->mCylinderVertexBuffer->getHandle(), mCIFData->mCylinderInstanceBuffer->getHandle() };
                commandBuffer.bindVertexBuffers(0, 2, vertexBuffers.data(), offsets);
                commandBuffer.bindIndexBuffer(mCIFData->mCylinderIndexBuffer->getHandle(), 0, vk::IndexType::eUint32);
                commandBuffer.drawIndexed(mCIFData->mCID.cylinder->getIndexCount(), mCIFData->mCID.cylinderTransforms.size(), 0, 0, 0);
            }
        });

        commandList->endLabel();
    }

    rg::NodeCreateInfo StructurePass::getNodeInfo() noexcept
    {
        using namespace rg;
        return {
            .nodeType     = NodeType::MolStructurePass,
            .displayName  = "Structure Pass",
            .subTitle     = "Molecule Rendering",
            .dependencies = {
                DependencyInfo {
                    .name           = res::rScene,
                    .dependencyType = DependencyType::Read,
                    .resourceType   = ResourceType::SceneData,
                },
                DependencyInfo {
                    .name           = res::rStructureRender,
                    .dependencyType = DependencyType::Write,
                    .resourceType   = ResourceType::Texture2D,
                    .resourceParams = ImageInfo {
                        .imageUsage = RHI::ImageUsage::ColorAttachment,
                        .format     = vk::Format::eR32G32B32A32Sfloat,
                    }
                },
            },
        };
    }
}
