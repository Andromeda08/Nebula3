#include "App.hpp"

#include "Configuration.hpp"
#include "UserInterface/Components/StatisticsComponent.hpp"
#include "VulkanRHI/Barrier.hpp"

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
    mUserInterface->addComponent<StatisticsComponent>();
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
        // Input
        mWindow->pollEvents();

        // Updates
        const float dt = mDeltaTime.getDeltaTime();
        mUserInterface->update();

        // Rendering
        const RHI::FrameData frameInfo   = mVulkanRHI->beginFrame();
        RHI::CommandList*    commandList = commandLists[frameInfo.currentFrame];

        commandList->begin();

        const SPtr<RHI::Image> currentSwapchainImage = mVulkanRHI->getSwapchain()->getImage(frameInfo.acquiredIndex);

        RHI::Barrier()
            .addImageBarrier({ RHI::ImageUsage::Clear, currentSwapchainImage })
            .insert(commandList);

        commandList->getHandle().clearColorImage(
            currentSwapchainImage->getImage(), vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue().setFloat32({ 0.8f, 0.2f, 1.0f }),
            currentSwapchainImage->getProperties().subresourceRange);

        RHI::Barrier()
             .addImageBarrier({ RHI::ImageUsage::ColorAttachment, currentSwapchainImage })
             .insert(commandList);

        mUserInterface->draw(commandList, frameInfo);

        RHI::Barrier()
             .addImageBarrier({ RHI::ImageUsage::PresentSrc, currentSwapchainImage })
             .insert(commandList);

        commandList->end();
        mVulkanRHI->endFrame_submitAndPresent({
            .frameData    = frameInfo,
            .pCommandList = commandList,
        });
    }
}
