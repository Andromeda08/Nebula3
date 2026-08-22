#include <rhi/DynamicRHI.hpp>
#include <rhi/vulkan/VulkanRHI.hpp>
#include <test/TestWindow.hpp>

using namespace sunflower;

int main()
{
    const auto window = TestWindow::create({
        .title = "Sunflower Test",
        .size  = { 1280, 720, 1 },
    });

    SPtr<rhi::DynamicRHI> rhi = rhi::VulkanRHI::create({
        .pWindow = window.get(),
    });

    const auto texture = rhi->createTexture({
        .size      = { 1280, 720, 1 },
        .mipLevels = 1u,
        .format    = rhi::Format::RGBA32_Float,
        .usage     = rhi::TextureUsage::ColorTarget,
        .type      = rhi::TextureType::e2D,
        .label     = "TestTexture",
    });

    UPtr<rhi::ICommandPool> graphicsCommandPool = rhi->getGraphicsQueue()->createCommandPool();

    PerFrameArray<rhi::ICommandList*> commandLists;
    for (auto& commandList : commandLists)
    {
        commandList = graphicsCommandPool->allocate();
    }

    bool mRunning = true;
    while (mRunning)
    {
        // Input
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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
        const rhi::FrameInfo frameInfo = rhi->beginFrame();
        auto* commandList = rhi_cast<rhi::VulkanCommandList>(commandLists[frameInfo.currentFrameIndex]);

        commandList->begin();

        auto* pImage = rhi_cast<rhi::VulkanTexture>(frameInfo.pSwapchainTexture);

        auto barrier = vk::ImageMemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eClear)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(pImage->getHandle())
            .setSubresourceRange(pImage->getSubresourceRange());

        commandList->getHandle().pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barrier));
        commandList->getHandle().clearColorImage(
            pImage->getHandle(),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ClearColorValue { 0.1f, 0.1f, 0.5f, 1.0f },
            pImage->getSubresourceRange());

        std::swap(barrier.srcStageMask, barrier.dstStageMask);
        std::swap(barrier.srcAccessMask, barrier.dstAccessMask);
        std::swap(barrier.oldLayout, barrier.newLayout);
        barrier
            .setDstStageMask(vk::PipelineStageFlagBits2::eNone)
            .setDstAccessMask(vk::AccessFlagBits2::eNone)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR);

        commandList->getHandle().pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barrier));

        commandList->end();

        rhi->endFrame_submitAndPresent({
            .frameInfo    = frameInfo,
            .pCommandList = commandList,
        });
    }

    rhi->getGraphicsQueue()->waitIdle();

    return 0;
} 