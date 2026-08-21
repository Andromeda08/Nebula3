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

    return 0;
} 