#include "App.hpp"

#include "Configuration.hpp"

App* gApplication = nullptr;

App::App()
{
    const auto& config = Configuration::getConfig();
    mWindow = Window::create({
        .size  = config.app.windowSize,
        .title = config.app.windowTitle,
    });
    mVulkanRHI = RHI::VulkanRHI::create({
        .pWindow = mWindow,
    });
    mUserInterface = UserInterface::create({
        .fontFile = "Resources/Fonts/GeistMono-Regular.ttf",
        .window   = mWindow,
        .rhi      = mVulkanRHI,
    });
}

UPtr<App> App::create() noexcept
{
    return std::make_unique<App>();
}

void App::run()
{
    mDeltaTime.initialize();

    const SPtr<RHI::CommandPool> graphicsCommandPool = mVulkanRHI->getGraphicsQueue()->createCommandPool();

    PerFrameArray<RHI::CommandList*> commandLists;
    for (auto i = 0; i < commandLists.size(); i++)
    {
        commandLists[i] = graphicsCommandPool->allocate();
    }

    // Main Loop
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();

        const RHI::FrameData frameInfo   = mVulkanRHI->beginFrame();
        RHI::CommandList*    commandList = commandLists[frameInfo.currentFrame];

        commandList->begin();

        const SPtr<RHI::Image> currentSwapchainImage = mVulkanRHI->getSwapchain()->getImage(frameInfo.acquiredIndex);

        {
            auto barrier = vk::ImageMemoryBarrier2()
                .setImage(currentSwapchainImage->getImage())
                .setSubresourceRange(currentSwapchainImage->getProperties().subresourceRange)
                .setOldLayout(currentSwapchainImage->getState().layout)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setDstStageMask(vk::PipelineStageFlagBits2::eClear);

            auto dependencyInfo = vk::DependencyInfo()
                .setImageMemoryBarrierCount(1)
                .setPImageMemoryBarriers(&barrier);

            commandList->getHandle().pipelineBarrier2(dependencyInfo);
        }

        commandList->getHandle().clearColorImage(
            currentSwapchainImage->getImage(), vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue().setFloat32({ 0.8f, 0.2f, 1.0f }),
            currentSwapchainImage->getProperties().subresourceRange);

        {
            auto barrier = vk::ImageMemoryBarrier2()
                .setImage(currentSwapchainImage->getImage())
                .setSubresourceRange(currentSwapchainImage->getProperties().subresourceRange)
                .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eClear)
                .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

            auto dependencyInfo = vk::DependencyInfo()
                .setImageMemoryBarrierCount(1)
                .setPImageMemoryBarriers(&barrier);

            commandList->getHandle().pipelineBarrier2(dependencyInfo);
        }

        mUserInterface->draw(commandList, frameInfo);

        {
            auto barrier = vk::ImageMemoryBarrier2()
                .setImage(currentSwapchainImage->getImage())
                .setSubresourceRange(currentSwapchainImage->getProperties().subresourceRange)
                .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
                .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe);

            auto dependencyInfo = vk::DependencyInfo()
                .setImageMemoryBarrierCount(1)
                .setPImageMemoryBarriers(&barrier);

            commandList->getHandle().pipelineBarrier2(dependencyInfo);
        }

        commandList->end();
        mVulkanRHI->endFrame_submitAndPresent({
            .frameData    = frameInfo,
            .pCommandList = commandList,
        });
    }
}
