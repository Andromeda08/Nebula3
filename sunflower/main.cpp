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

    return 0;
} 