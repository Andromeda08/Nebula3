add_executable(Sunflower
    main.cpp
    rhi/vulkan/VulkanRHI.hpp
    rhi/DynamicRHI.hpp
    rhi/Common.hpp
    rhi/vulkan/detail/Instance.hpp
    rhi/vulkan/VulkanRHI.cpp
    rhi/vulkan/VulkanCommon.hpp
    rhi/IWindow.hpp
    rhi/vulkan/detail/Instance.cpp
    rhi/vulkan/Device.hpp
    rhi/vulkan/detail/ExtensionDef.hpp
    rhi/vulkan/detail/ExtensionDef.cpp
    rhi/vulkan/Device.cpp
    rhi/vulkan/detail/Surface.cpp
    rhi/vulkan/detail/Surface.hpp

    test/TestWindow.hpp
    test/TestWindow.cpp
    rhi/vulkan/impl/vma.impl.cpp
    rhi/vulkan/Texture.cpp
    rhi/vulkan/Texture.hpp
    rhi/DynamicRHI.cpp
    rhi/vulkan/Swapchain.hpp
    rhi/vulkan/Swapchain.cpp
    rhi/vulkan/detail/VulkanTimelineSync.hpp
    rhi/vulkan/detail/VulkanTimelineSync.cpp
    rhi/vulkan/CommandQueue.hpp
    rhi/vulkan/CommandQueue.cpp
    rhi/vulkan/CommandPool.hpp
    rhi/vulkan/CommandList.hpp
    rhi/vulkan/CommandPool.cpp
    rhi/vulkan/CommandList.cpp
)

target_include_directories(Sunflower PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ../sunflower
    ../External/glm
    ../External/vma/include
    ../External/SDL/include
    ../External/spdlog/include
)

target_link_libraries(Sunflower PUBLIC
    ${Vulkan_LIBRARIES}
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
    spdlog::spdlog
    slang::slang
)

target_compile_definitions(Sunflower PUBLIC
    SUNFLOWER_DEBUG
    SUNFLOWER_MOLTEN_VK
    VULKAN_HPP_NO_EXCEPTIONS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
