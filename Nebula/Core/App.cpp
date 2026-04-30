#include "App.hpp"

#include "Configuration.hpp"
#include "Scene/SceneV2.hpp"
#include "Scene/Components/SceneInfoComponent.hpp"
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

    mTextureManager = TextureManager::create({
        .rhi = mVulkanRHI,
    });

    SplashWindow::get().setMessage("Initializing UserInterface...");
    mUserInterface = UserInterface::create({
        .fontFile = "GeistMono-Regular.ttf",
        .window   = mWindow,
        .rhi      = mVulkanRHI,
    });
    mUserInterface->addComponent<StatisticsComponent>(mVulkanRHI, &mCPUFramerate);

    // mScene = makeUnique<SceneV2>(mVulkanRHI, mTextureManager.get(), mUserInterface.get());
    // mUserInterface->addComponent<SceneInfoComponent>(mScene.get());

    mLevel = makeUnique<nbl::Level>(mVulkanRHI, mUserInterface.get(), mTextureManager.get());
    mLevelRenderer = makeUnique<nbl::LevelRenderer>(mVulkanRHI, mTextureManager.get(), mLevel.get());

    {
        const auto [w, h] = mWindow->getFramebufferSize();
        mTitleScreen = makeUnique<TitleScreen>(glm::vec2(w, h), mVulkanRHI, mTextureManager.get());
    }

    mWindow->reveal();
}

UPtr<App> App::create() noexcept
{
    return std::make_unique<App>();
}

App::~App()
{
    mVulkanRHI->getDevice()->waitIdle();
}

void App::run()
{
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

        // Input
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Look for Gamepad connection events
            mGamepadManager->onGamepadEvent(event);

            // Let ImGui process events
            mUserInterface->processEvents(event);
            // If ImGui didn't want to consume any input continue with Scene handlers.
            if (!UserInterface::wantCaptureInput())
            {
                if (mLevel)
                {
                    mLevel->onEvent(event);
                }
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
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    mRunning = false;
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

        // =====================================
        // Updates
        // =====================================
        commandList->beginLabel("Updates");

        mTextureManager->update(commandList);
        if (mLevel)
        {
            mLevel->onUpdate(dt, frameData, commandList);
        }
        mUserInterface->update();

        commandList->endLabel();

        // =====================================
        // Rendering
        // =====================================
        commandList->beginLabel("Rendering");

        mLevelRenderer->render(frameData, commandList);
        mTitleScreen->render(commandList, frameData);

        commandList->endLabel();

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
    }
}
