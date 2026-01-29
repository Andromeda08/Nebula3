#include "App.hpp"

#include "Configuration.hpp"
#include "RenderGraph/Editor/RenderGraphEditorComponent.hpp"
#include "RenderPass/HelloTrianglePass.hpp"
#include "Scene/Components/SceneInfoComponent.hpp"
#include "../Scene/Scenes/MoleculeScene/MoleculeScene.hpp"
#include "UserInterface/Components/StatisticsComponent.hpp"
#include "VulkanRHI/Barrier.hpp"

App* gApplication = nullptr;

App::App()
{
    const auto& config = Configuration::getConfig();
    mWindow = SDLWindow::create({
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

    mScene = makeUnique<MoleculeScene>(SceneCreateInfo{
        .rhi  = mVulkanRHI,
        .name = "Molecule Scene",
    });
    mUserInterface->addComponent<SceneInfoComponent>(mScene.get());
    MoleculeScene::registerUIComponent(dynamic_cast<MoleculeScene*>(mScene.get()), mUserInterface.get());
}

UPtr<App> App::create() noexcept
{
    return std::make_unique<App>();
}

void App::run_renderPathLoop()
{
    // Initialize variables, start main loop
    mRunning = true;
    mDeltaTime.initialize();

    const SPtr<RHI::CommandPool> graphicsCommandPool = mVulkanRHI->getGraphicsQueue()->createCommandPool();

    PerFrameArray<RHI::CommandList*> commandLists;
    for (auto& commandList : commandLists)
    {
        commandList = graphicsCommandPool->allocate();
    }

    const auto helloTrianglePass = std::make_unique<HelloTrianglePass>(mVulkanRHI);

    // Main Loop
    while (mRunning)
    {
        const float dt = mDeltaTime.getDeltaTime();
        // auto* pRenderPath = mRenderGraphContext->getCurrentRenderPath();

        // Input
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Let ImGui process events
            mUserInterface->processEvents(event);
            // If ImGui didn't want to consume any input continue with Scene handlers.
            if (!mUserInterface->wantCaptureInput())
            {
                mScene->onEvent(event);
            }

            switch (event.type)
            {
                case SDL_EVENT_KEY_DOWN: {
                    const SDL_KeyboardEvent& keyboardEvent = event.key;
                    if (keyboardEvent.key == SDLK_ESCAPE)
                    {
                        mRunning = false;
                    }
                    break;
                }
                default: {}
            }
        }

        // Rendering
        const RHI::FrameData frameData   = mVulkanRHI->beginFrame();
        RHI::CommandList*    commandList = commandLists[frameData.currentFrame];

        // Updates
        mScene->onUpdate(commandList, frameData, dt);
        // pRenderPath->update(dt, frameData);
        mUserInterface->update();

        commandList->begin();

        mVulkanRHI->getSwapchain()->setScissorViewport(commandList->getHandle());

        // =====================================
        // RenderPath
        // =====================================
        // pRenderPath->initialize(commandList);   // Runs once
        // pRenderPath->execute(commandList, frameData);

        /* Acquired Swapchain Image | ColorAttachment */ {
            const auto barrier = RHI::Barrier()
                .addBarrier(mVulkanRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::ColorAttachment));
            barrier.insert(commandList);
        }
        mScene->render(commandList, frameData);

        // =====================================
        // User Interface
        // =====================================
        commandList->getHandle().beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT().setPLabelName("ImGui"));

        /* Acquired Swapchain Image | ColorAttachment */ {
            const auto barrier = RHI::Barrier()
                .addBarrier(mVulkanRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::ColorAttachment));
            barrier.insert(commandList);
        }
        mUserInterface->draw(commandList, frameData);
        /* Acquired Swapchain Image | PresentSrc */ {
            const auto barrier = RHI::Barrier()
                .addBarrier(mVulkanRHI->getSwapchain()->getBarrier(frameData.acquiredIndex, RHI::ImageUsage::PresentSrc));
            barrier.insert(commandList);
        }

        commandList->getHandle().endDebugUtilsLabelEXT();

        commandList->end();
        mVulkanRHI->endFrame_submitAndPresent({
            .frameData    = frameData,
            .pCommandList = commandList,
        });

        // if (mRenderGraphContext->hasQueuedRenderPathChange())
        // {
        //     mRenderGraphContext->changeToQueuedRenderPath();
        // }
    }
}
