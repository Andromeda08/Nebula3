#include "SDFRaymarchPass.hpp"

#include "VulkanRHI/Barrier.hpp"

namespace Molecule
{
    SDFRaymarchPass::SDFRaymarchPass(const SPtr<RHI::VulkanRHI>& rhi, const SPtr<RHI::Descriptor>& sceneDescriptor, const SPtr<RHI::Texture>& sdfTexture)
    : Pass()
    , mRHI(rhi)
    , mSDFTexture(sdfTexture)
    , mSceneDescriptor(sceneDescriptor)
    {
        createSampler();

        mDescriptor = mRHI->createDescriptor({
            .bindings = {
                vk::DescriptorSetLayoutBinding { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
            },
            .setCount = 1,
            .debugName = "SDFRaymarchDescriptor",
        });

        // TODO: Update when there are 3D Images in RHI::DescriptorWrite
        const auto imageInfo = vk::DescriptorImageInfo { mSampler, mSDFTexture->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal };
        const auto descriptorWrite = RHI::DescriptorWrite()
            .writeCombinedImageSamplerInfos(0, std::span{&imageInfo, 1});
        mDescriptor->writeAll(descriptorWrite);

        const auto area = mRHI->getSwapchain()->getProperties().area;
        mRenderPass = mRHI->createRenderPass({
            .renderArea = RHI2::Rect2D().setOffset({ area.offset.x, area.offset.y }).setExtent({ area.extent.width, area.extent.height }),
            .colorAttachments = { RHI::Attachment {
                .image = mRHI->getSwapchain()->getImage(0),
                .attachmentInfo = vk::RenderingAttachmentInfo()
                    .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setImageView(mRHI->getSwapchain()->getImageView(0))
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                }
            },
            .label = "SDFRaymarchRenderPass",
        });

        RHI::GraphicsPipelineCreateInfo(pipelineInfo) = RHI::GraphicsPipelineCreateInfo()
            .setPushConstantRange({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(SDFRaymarchParams) })
            .addDescriptorSetLayout(mSceneDescriptor->getLayout())
            .addDescriptorSetLayout(mDescriptor->getLayout())
            .setStateInfo(RHI::GraphicsPipelineStateInfo()
                .setCullMode(vk::CullModeFlagBits::eNone)
                .addAttachmentState(RHI::PipelineUtils::makeColorBlendAttachmentState()
                    .setBlendEnable(true)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)))
            .addShader({ "Resources/Shaders/bin/FSQuad.vert.spv", vk::ShaderStageFlagBits::eVertex })
            .addShader({ "Resources/Shaders/bin/SDF.frag.spv", vk::ShaderStageFlagBits::eFragment })
            .addColorAttachmentFormat(mRHI->getSwapchain()->getProperties().format)
            .setDebugName("SDFRaymarchPipeline");

        mPipeline = mRHI->createGraphicsPipeline(pipelineInfo);
    }

    void SDFRaymarchPass::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData)
    {
        commandList->beginLabel("SDFRaymarchPass");

        const auto barrier = RHI::Barrier()
                .addBarrier(mSDFTexture->getBarrier(RHI::ImageUsage::ShaderReadOnly));
        barrier.insert(commandList);

        // ❗Don't clear previous render pass result
        const auto colorAttachment = RHI::Attachment{
            .image = mRHI->getSwapchain()->getImage(frameData.acquiredIndex),
            .attachmentInfo = vk::RenderingAttachmentInfo()
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setImageView(mRHI->getSwapchain()->getImageView(frameData.acquiredIndex))
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
        };
        mRenderPass->setColorAttachment(0, colorAttachment);

        mRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer) -> void {
            mPipeline->bind(commandBuffer);
            mPipeline->pushConstants(commandBuffer, &mParams);
            mPipeline->bindDescriptorSets(commandBuffer, { mSceneDescriptor->getSet(frameData.currentFrame), mDescriptor->getSet(0) });
            commandBuffer.draw(3, 1, 0, 0);
        });

        commandList->endLabel();
    }

    rg::NodeCreateInfo SDFRaymarchPass::getNodeInfo() noexcept
    {
        using namespace rg;
        return {
            .nodeType     = NodeType::MolSDFRaymarchPass,
            .displayName  = "SDF Raymarch Pass",
            .subTitle     = "Molecule Rendering",
            .dependencies = {
                DependencyInfo {
                    .name           = res::rScene,
                    .dependencyType = DependencyType::Read,
                    .resourceType   = ResourceType::SceneData,
                },
                DependencyInfo {
                    .name           = res::rSDFTexture,
                    .dependencyType = DependencyType::Read,
                    .resourceType   = ResourceType::Texture3D,
                    .resourceParams = ImageInfo {
                        .imageUsage = RHI::ImageUsage::ShaderReadOnly,
                        .format     = vk::Format::eR32G32B32A32Sfloat,
                        .extent     = vk::Extent3D { 128, 128, 128 },
                    }
                },
                DependencyInfo {
                    .name           = res::rStructureRender,
                    .dependencyType = DependencyType::Read,
                    .resourceType   = ResourceType::Texture2D,
                    .resourceParams = ImageInfo {
                        .imageUsage = RHI::ImageUsage::ColorAttachment,
                        .format     = vk::Format::eR32G32B32A32Sfloat,
                    }
                },
                DependencyInfo {
                    .name           = res::rFinalRender,
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

    void SDFRaymarchPass::createSampler()
    {
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

        mSampler = mRHI->getDevice()->getHandle().createSampler(samplerCreateInfo);
    }
}
