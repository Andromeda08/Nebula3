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
}

UPtr<App> App::create() noexcept
{
    return std::make_unique<App>();
}

void App::run()
{
    mDeltaTime.initialize();

    auto graphicsCommandPool = mVulkanRHI->getGraphicsQueue()->createCommandPool();

    PerFrameArray<RHI::CommandList*> commandLists;
    for (auto i = 0; i < commandLists.size(); i++)
    {
        commandLists[i] = graphicsCommandPool->allocate();
    }

    // Main Loop
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();

        auto frameInfo = mVulkanRHI->beginFrame();
        auto* commandList = commandLists[frameInfo.currentFrame];
        commandList->begin();

        auto currentSwapchainImage = mVulkanRHI->getSwapchain()->getImage(frameInfo.acquiredIndex);

        {
            auto barrier = vk::ImageMemoryBarrier2()
                .setImage(currentSwapchainImage->getImage())
                .setSubresourceRange(currentSwapchainImage->getProperties().subresourceRange)
                .setOldLayout(currentSwapchainImage->getState().layout)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal);

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
                .setNewLayout(vk::ImageLayout::ePresentSrcKHR);

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
