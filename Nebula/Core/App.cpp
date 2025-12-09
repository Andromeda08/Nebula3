#include "App.hpp"

#include "Configuration.hpp"
#include "RenderGraph/Editor/RenderGraphEditorComponent.hpp"
#include "RenderPass/HelloTrianglePass.hpp"
#include "Scene/Components/SceneInfoComponent.hpp"
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

    mRenderGraphContext = rg::RenderGraphContext::create({
        .rhi = mVulkanRHI,
    });
    mUserInterface->addComponent<rg::RenderGraphEditorComponent>(mRenderGraphContext);

    mScene = Scene::create({
        .rhi  = mVulkanRHI,
        .name = "Default Scene",
    });
    mUserInterface->addComponent<SceneInfoComponent>(mScene.get());
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

    const auto helloTrianglePass = std::make_unique<HelloTrianglePass>(mVulkanRHI);

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

        {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::ColorAttachment, currentSwapchainImage });
            barrier.insert(commandList);
        }

        // commandList->getHandle().clearColorImage(
        //     currentSwapchainImage->getImage(), vk::ImageLayout::eTransferDstOptimal,
        //     vk::ClearColorValue().setFloat32({ 0.8f, 0.2f, 1.0f }),
        //     currentSwapchainImage->getProperties().subresourceRange);

        mVulkanRHI->getSwapchain()->setScissorViewport(commandList->getHandle());
        helloTrianglePass->execute(commandList, frameInfo);

        {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::ColorAttachment, currentSwapchainImage });
             barrier.insert(commandList);
        }

        mUserInterface->draw(commandList, frameInfo);

        {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::PresentSrc, currentSwapchainImage });
             barrier.insert(commandList);
        }

        commandList->end();
        mVulkanRHI->endFrame_submitAndPresent({
            .frameData    = frameInfo,
            .pCommandList = commandList,
        });
    }
}

void App::run_renderPathLoop()
{
    mDeltaTime.initialize();

    const SPtr<RHI::CommandPool> graphicsCommandPool = mVulkanRHI->getGraphicsQueue()->createCommandPool();

    PerFrameArray<RHI::CommandList*> commandLists;
    for (auto i = 0; i < commandLists.size(); i++)
    {
        commandLists[i] = graphicsCommandPool->allocate();
    }

    const auto helloTrianglePass = std::make_unique<HelloTrianglePass>(mVulkanRHI);

    // Main Loop
    while (!mWindow->shouldClose())
    {
        const float dt = mDeltaTime.getDeltaTime();
        auto* pRenderPath = mRenderGraphContext->getCurrentRenderPath();

        // Input
        mWindow->pollEvents();

        // Rendering
        const RHI::FrameData frameData   = mVulkanRHI->beginFrame();
        RHI::CommandList*    commandList = commandLists[frameData.currentFrame];

        // Updates
        pRenderPath->update(dt, frameData);
        mUserInterface->update();

        commandList->begin();

        const SPtr<RHI::Image> currentSwapchainImage = mVulkanRHI->getSwapchain()->getImage(frameData.acquiredIndex);

        mVulkanRHI->getSwapchain()->setScissorViewport(commandList->getHandle());

        // =====================================
        // RenderPath
        // =====================================
        pRenderPath->initialize(commandList);   // Runs once

        pRenderPath->execute(commandList, frameData);

        // =====================================
        // User Interface
        // =====================================
        #pragma region
        {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::ColorAttachment, currentSwapchainImage });
             barrier.insert(commandList);
        }

        mUserInterface->draw(commandList, frameData);

        {
            auto barrier = RHI::Barrier()
                .addImageBarrier({ RHI::ImageUsage::PresentSrc, currentSwapchainImage });
             barrier.insert(commandList);
        }
        #pragma endregion

        commandList->end();
        mVulkanRHI->endFrame_submitAndPresent({
            .frameData    = frameData,
            .pCommandList = commandList,
        });

        if (mRenderGraphContext->hasQueuedRenderPathChange())
        {
            mRenderGraphContext->changeToQueuedRenderPath();
        }
    }
}
