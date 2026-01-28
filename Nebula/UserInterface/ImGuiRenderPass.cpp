#include "ImGuiRenderPass.hpp"

#include <imgui.h>
#include <imnodes.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include "VulkanRHI/Barrier.hpp"

ImGuiRenderPass::ImGuiRenderPass(const ImGuiRenderPassCreateInfo& createInfo)
: mFontFile(createInfo.fontFile)
, mWindow(createInfo.window)
, mRHI(createInfo.rhi)
, mDevice(createInfo.rhi->getDevice())
{
    mDebugLabel = vk::DebugUtilsLabelEXT()
        .setColor(std::array{ 0.8235f, 0.0588f, 0.2235f, 1.0f })
        .setPLabelName("ImGuiPass");

    init_VulkanResources();
    init_ImGui();
}

ImGuiRenderPass::~ImGuiRenderPass()
{
    mDevice->waitIdle();

    ImNodes::DestroyContext();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    mDevice->getHandle().destroy(mDescriptorPool);
}

void ImGuiRenderPass::render(const RHI::CommandList* pCommandList, const RHI::FrameData& frameData, const std::function<void()>& uiDraws) noexcept
{
    const auto imageIdx = frameData.acquiredIndex;
    const auto commandBuffer = pCommandList->getHandle();

    const auto attachment = vk::RenderingAttachmentInfo()
        .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 0.0f}))
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setImageView(mRHI->getSwapchain()->getImageView(imageIdx))
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore);
    mRenderingInfo.setColorAttachments(attachment);

    const auto barrier = RHI::Barrier()
        .addBarrier(mRHI->getSwapchain()->getBarrier(imageIdx, RHI::ImageUsage::ColorAttachment));

    commandBuffer.beginDebugUtilsLabelEXT(mDebugLabel);
    barrier.insert(commandBuffer);
    commandBuffer.beginRendering(mRenderingInfo);
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();
        uiDraws();
        ImGui::EndFrame();

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }
    commandBuffer.endRendering();
    commandBuffer.endDebugUtilsLabelEXT();
}

void ImGuiRenderPass::init_VulkanResources() noexcept
{
    constexpr auto poolSize = vk::DescriptorPoolSize()
        .setDescriptorCount(IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE)
        .setType(vk::DescriptorType::eCombinedImageSampler);

    const auto descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo()
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(poolSize.descriptorCount)
        .setPPoolSizes(&poolSize)
        .setPoolSizeCount(1);

    const auto result = mDevice->getHandle().createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &mDescriptorPool);
    assert(result == vk::Result::eSuccess);

    mRenderingInfo = vk::RenderingInfo()
        .setColorAttachmentCount(1)
        .setLayerCount(1)
        .setRenderArea({{0, 0}, mRHI->getSwapchain()->getProperties().extent});
}

void ImGuiRenderPass::init_ImGui() const noexcept
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(mFontFile.c_str(), 16.0f);

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);

    auto bResult = ImGui_ImplSDL3_InitForVulkan(mWindow->getHandle());
    assert(bResult);

    const auto format = static_cast<VkFormat>(mRHI->getSwapchain()->getProperties().format);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_4;
    initInfo.Instance = mRHI->getInstance()->getHandle();
    initInfo.PhysicalDevice = mDevice->getPhysicalDevice();
    initInfo.Device = mDevice->getHandle();
    initInfo.QueueFamily = mDevice->getGraphicsQueue().familyIndex;
    initInfo.Queue = mDevice->getGraphicsQueue().queue;
    initInfo.PipelineCache = mPipelineCache;
    initInfo.DescriptorPool = mDescriptorPool;
    initInfo.MinImageCount = mRHI->getSwapchain()->getImageCount();
    initInfo.ImageCount = mRHI->getSwapchain()->getImageCount();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = {},
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    bResult = ImGui_ImplVulkan_Init(&initInfo);
    assert(bResult);

    ImNodes::CreateContext();
    ImNodes::StyleColorsDark();
}
