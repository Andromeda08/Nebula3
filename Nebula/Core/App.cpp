#include "App.hpp"

#include "Configuration.hpp"
#include "RenderGraph/Editor/RenderGraphEditorComponent.hpp"
#include "RenderPass/HelloTrianglePass.hpp"
#include "Scene/SceneV2.hpp"
#include "Scene/Components/SceneInfoComponent.hpp"
#include "Scene/Scenes/MoleculeScene/MoleculeScene.hpp"
#include "Scene/Voxel/VoxelScene.hpp"
#include "UserInterface/Components/StatisticsComponent.hpp"
#include "VulkanRHI/Barrier.hpp"
#include "Window/SplashWindow.hpp"

App* gApplication = nullptr;

App::App()
{
    const auto& config = Configuration::getConfig();
    SplashWindow::get().setMessage("Creating Window...");
    mWindow = SDLWindow::create({
        .size  = config.app.windowSize,
        .title = config.app.windowTitle,
    });

    mGamepadManager = makeUnique<GamepadManager>();

    SplashWindow::get().setMessage("Initializing VulkanRHI...");
    mVulkanRHI = RHI::VulkanRHI::create({
        .pWindow = mWindow,
    });

    SplashWindow::get().setMessage("Initializing UserInterface...");
    mUserInterface = UserInterface::create({
        .fontFile = "Resources/Fonts/GeistMono-Regular.ttf",
        .window   = mWindow,
        .rhi      = mVulkanRHI,
    });
    mUserInterface->addComponent<StatisticsComponent>(mVulkanRHI, &mCPUFramerate);

    mScene = makeUnique<SceneV2>(mVulkanRHI, mUserInterface.get());
    mUserInterface->addComponent<SceneInfoComponent>(mScene.get());

    mWindow->reveal();
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

    // Main Loop
    while (mRunning)
    {
        const float dt = mDeltaTime.getDeltaTime();
        mCPUFramerate = dt;
        // auto* pRenderPath = mRenderGraphContext->getCurrentRenderPath();

        mScene->preFrame();

        // Input
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Look for Gamepad connection events
            mGamepadManager->onGamepadEvent(event);

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
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
                    spdlog::info("Pressed: {} ", event.gbutton.button);
                    break;
                }
                default: {
                    break;
                }
            }
        }

        // Rendering
        const RHI::FrameData frameData   = mVulkanRHI->beginFrame();
        RHI::CommandList*    commandList = commandLists[frameData.currentFrame];

        commandList->begin();

        // Updates
        mScene->onUpdate(dt, frameData, commandList);

        // pRenderPath->update(dt, frameData);
        mUserInterface->update();

        // =====================================
        // RenderPath
        // =====================================
        // pRenderPath->initialize(commandList);   // Runs once
        // pRenderPath->execute(commandList, frameData);

        mScene->onRender(commandList, frameData);

        // =====================================
        // User Interface
        // =====================================
        commandList->beginLabel("ImGui");

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

        commandList->endLabel();

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
