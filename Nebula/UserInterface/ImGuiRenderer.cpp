#include "ImGuiRenderer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "imgui_internal.h"

ImGuiRenderer::ImGuiRenderer(const ImGuiRendererCreateInfo& createInfo)
: mFontFile(createInfo.fontFile)
, mWindow(createInfo.window)
, mRHI(createInfo.rhi)
{
    createDescriptorPool();
    createRenderPass();
    createFramebuffer();
    initImGuiContext();

    mDebugLabel = vk::DebugUtilsLabelEXT()
        .setColor(std::array{ 0.8235f, 0.0588f, 0.2235f, 1.0f })
        .setPLabelName("ImGui");
}

ImGuiRenderer::~ImGuiRenderer()
{
    const vk::Device device = mRHI->getDevice()->getHandle();
    device.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (const auto& framebuffer : mFramebuffers)
    {
        device.destroy(framebuffer);
    }
    device.destroyRenderPass(mRenderPass);
    device.destroyDescriptorPool(mDescriptorPool);
}

void ImGuiRenderer::render(
    const vk::CommandBuffer&        commandBuffer,
    const RHI::FrameData&           frameData,
    const std::function<void()>&    uiDraws)
{
    mRenderPassBeginInfo.setFramebuffer(mFramebuffers[frameData.currentFrame]);

    commandBuffer.beginDebugUtilsLabelEXT(mDebugLabel);

    /* Barrier -> ColorAttachmentOptimal */ {
        const auto currentSwapchainImage = mRHI->getSwapchain()->getImage(frameData.acquiredIndex);
        const auto barrier = vk::ImageMemoryBarrier2()
            .setImage(currentSwapchainImage->getImage())
            .setSubresourceRange(currentSwapchainImage->getProperties().subresourceRange)
            .setOldLayout(currentSwapchainImage->getState().layout)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        const auto dependencyInfo = vk::DependencyInfo()
            .setImageMemoryBarrierCount(1)
            .setPImageMemoryBarriers(&barrier);

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    commandBuffer.beginRenderPass(&mRenderPassBeginInfo, vk::SubpassContents::eInline);
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        uiDraws();
        ImGui::EndFrame();

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }
    commandBuffer.endRenderPass();
    commandBuffer.endDebugUtilsLabelEXT();
}

void ImGuiRenderer::createDescriptorPool()
{
    #pragma region "Descriptor pool sizes"
    constexpr vk::DescriptorPoolSize poolSizes[] {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };
    #pragma endregion

    const auto descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo()
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(1000 * IM_ARRAYSIZE(poolSizes))
        .setPPoolSizes(poolSizes)
        .setPoolSizeCount(IM_ARRAYSIZE(poolSizes));

    const vk::Result result = mRHI
        ->getDevice()
        ->getHandle()
        .createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &mDescriptorPool);
    assert(result == vk::Result::eSuccess);
}

void ImGuiRenderer::createRenderPass()
{
    mClearValue = vk::ClearValue().setColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    const auto attachment = vk::AttachmentDescription()
        .setFormat(mRHI->getSwapchain()->getProperties().format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    constexpr auto attachmentRef = vk::AttachmentReference()
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setAttachment(0);

    const auto subpass = vk::SubpassDescription()
        .setColorAttachmentCount(1)
        .setPColorAttachments(&attachmentRef)
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    constexpr auto subpassDependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setSrcAccessMask({})
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);

    const auto renderPassCreateInfo = vk::RenderPassCreateInfo()
        .setAttachmentCount(1)
        .setPAttachments(&attachment)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&subpassDependency);

    const vk::Result result = mRHI
        ->getDevice()
        ->getHandle()
        .createRenderPass(&renderPassCreateInfo, nullptr, &mRenderPass);
    assert(result == vk::Result::eSuccess);

    mRHI->getDevice()->nameObject<vk::RenderPass>({
        .debugName = "ImGui-RenderPass",
        .handle    = mRenderPass,
    });

    mRenderArea = mRHI->getSwapchain()->getProperties().area;
    mRenderPassBeginInfo = vk::RenderPassBeginInfo()
        .setRenderArea(mRenderArea)
        .setRenderPass(mRenderPass)
        .setClearValueCount(1)
        .setPClearValues(&mClearValue);
}

void ImGuiRenderer::createFramebuffer()
{
    auto framebufferCreateInfo = vk::FramebufferCreateInfo()
        .setAttachmentCount(1)
        .setHeight(mRenderArea.extent.height)
        .setWidth(mRenderArea.extent.width)
        .setLayers(1)
        .setRenderPass(mRenderPass);

    const auto* pSwapchain = mRHI->getSwapchain();
    for (uint32_t i = 0; i < mFramebuffers.size(); i++)
    {
        auto imageView = pSwapchain->getImageView(i);
        framebufferCreateInfo.setPAttachments(&imageView);

        const vk::Result result = mRHI
            ->getDevice()
            ->getHandle()
            .createFramebuffer(&framebufferCreateInfo, nullptr, &mFramebuffers[i]);
        assert(result == vk::Result::eSuccess);

        mRHI->getDevice()->nameObject<vk::Framebuffer>({
            .debugName = std::format("ImGui-Framebuffer[{}]", i),
            .handle    = mFramebuffers[i],
        });
    }
}

void ImGuiRenderer::initImGuiContext() const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(mFontFile.c_str(), 16.0f);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);

    const RHI::Device* pDevice = mRHI->getDevice();

    ImGui_ImplGlfw_InitForVulkan(mWindow->getHandle(), true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = mRHI->getInstance()->getHandle();
    initInfo.PhysicalDevice = pDevice->getPhysicalDevice();
    initInfo.Device = pDevice->getHandle();
    initInfo.QueueFamily = pDevice->getGraphicsQueue().familyIndex;
    initInfo.Queue = pDevice->getGraphicsQueue().queue;
    initInfo.PipelineCache = mPipelineCache;
    initInfo.DescriptorPool = mDescriptorPool;
    initInfo.ImageCount = gFramesInFlight;
    initInfo.MinImageCount = gFramesInFlight;
    initInfo.ApiVersion = VK_API_VERSION_1_4;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.PipelineInfoMain = ImGui_ImplVulkan_PipelineInfo {
        .RenderPass  = mRenderPass,
        .Subpass     = 0,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    };
    ImGui_ImplVulkan_Init(&initInfo);
}
