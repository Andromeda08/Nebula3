#include "GBufferPass.hpp"

#include <glm/glm.hpp>
#include "RenderGraph/RenderPath/Resource.hpp"
#include "Scene/GPUObjectInstanceData.hpp"
#include "Scene/Vertex.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

GBufferPass::GBufferPass(const GBufferPassCreateInfo& createInfo)
: IPass()
, mRHI(createInfo.rhi)
{
}

void GBufferPass::initialize()
{
    mScene = getResource(res::rScene)->as<rg::SceneDataResource>();
    assert(mScene);

    const auto* position = getResource(res::rPositionBuffer)->as<rg::ImageResource>();
    assert(position);

    const auto* normal = getResource(res::rNormalBuffer)->as<rg::ImageResource>();
    assert(normal);

    const auto* albedo = getResource(res::rAlbedoBuffer)->as<rg::ImageResource>();
    assert(albedo);

    mRenderPass = mRHI->createRenderPass({
        .colorAttachments = {
            position->getBasicAttachment(), normal->getBasicAttachment(), albedo->getBasicAttachment()
        },
        .label = sName
    });

    const auto pipelineCreateInfo = RHI::GraphicsPipelineCreateInfo()
        .setDebugName(sName)
        .addColorAttachmentFormats({ position->getFormat(), normal->getFormat(), albedo->getFormat() })
        .addShader({
            .filePath    = Configuration::getShaderFilePath("GBuffer.vert.spv"),
            .shaderStage = vk::ShaderStageFlagBits::eVertex,
        })
        .addShader({
            .filePath    = Configuration::getShaderFilePath("GBuffer.frag.spv"),
            .shaderStage = vk::ShaderStageFlagBits::eFragment,
        })
        .setStateInfo(RHI::GraphicsPipelineStateInfo()
            .addAttachmentState()
            .addAttachmentState()
            .addAttachmentState()
            .configure([](RHI::GraphicsPipelineStateInfo& state) -> void {
                state
                    .addVertexType<Vertex>(0)
                    .addVertexType<GPUObjectInstanceData>(1);
            }));

    mPipeline = mRHI->createGraphicsPipeline(pipelineCreateInfo);
}

void GBufferPass::execute(const RHI::CommandList* commandList, const RHI::FrameData& frameData)
{
    mRenderPass->execute(commandList->getHandle(), [&](const vk::CommandBuffer& commandBuffer)
    {
        mPipeline->bind(commandBuffer);
        // mScene->render(commandBuffer);
    });
}

void GBufferPass::setResource(const std::string& name, const SPtr<rg::Resource>& resource)
{
    mResources.insert_or_assign(name, resource);
}

rg::Resource* GBufferPass::getResource(const std::string& name)
{
    return mResources[name].get();
}

rg::NodeCreateInfo GBufferPass::getNodeInfo() noexcept
{
    using namespace rg;
    return {
        .nodeType     = sType,
        .displayName  = sName,
        .dependencies = {
            DependencyInfo {
                .name           = res::rScene,
                .dependencyType = DependencyType::Read,
                .resourceType   = ResourceType::SceneData,
            },
            DependencyInfo {
                .name           = res::rPositionBuffer,
                .dependencyType = DependencyType::Write,
                .resourceType   = ResourceType::Image,
                .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
            },
            DependencyInfo {
                .name           = res::rNormalBuffer,
                .dependencyType = DependencyType::Write,
                .resourceType   = ResourceType::Image,
                .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
            },
            DependencyInfo {
                .name           = res::rAlbedoBuffer,
                .dependencyType = DependencyType::Write,
                .resourceType   = ResourceType::Image,
                .resourceParams = ImageInfo { RHI::ImageUsage::ColorAttachment },
            },
        },
    };
}
