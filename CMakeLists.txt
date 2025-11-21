cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/json)
add_subdirectory(External/vma)

add_executable(Nebula3
    Nebula/Core/Configuration.hpp
    Nebula/VulkanRHI/RHIConfiguration.hpp
    Nebula/Core/AppSpecification.hpp
    Nebula/Core/Types.hpp
    Nebula/Core/App.hpp
    Nebula/main.cpp
    Nebula/Core/Configuration.cpp
    Nebula/Core/Macro.hpp
    Nebula/Core/App.cpp
    Nebula/Window/Window.hpp
    Nebula/Math/DeltaTime.hpp
    Nebula/Math/DeltaTime.cpp
    Nebula/VulkanRHI/IWindow.hpp
    Nebula/Window/Window.cpp
    Nebula/Core/ToString.hpp
    Nebula/VulkanRHI/VulkanRHI.hpp
    Nebula/Math/Transform.hpp
    Nebula/Math/Gradient.hpp
    Nebula/Math/MathUtil.hpp
    Nebula/VulkanRHI/Frame.hpp
    Nebula/VulkanRHI/Device.hpp
    Nebula/VulkanRHI/VulkanCore.hpp
    Nebula/VulkanRHI/VulkanRHI.cpp
    Nebula/Core/Util.hpp
    Nebula/Impl/vma.impl.cpp
    Nebula/VulkanRHI/Device.cpp
    Nebula/Impl/std_macos.pch.hpp
    Nebula/VulkanRHI/Swapchain.hpp
    Nebula/VulkanRHI/Instance.hpp
    Nebula/VulkanRHI/Instance.cpp
    Nebula/VulkanRHI/Swapchain.cpp
    Nebula/VulkanRHI/Image.hpp
    Nebula/VulkanRHI/Image.cpp
    Nebula/VulkanRHI/Buffer.hpp
    Nebula/VulkanRHI/Buffer.cpp
    Nebula/VulkanRHI/DebugContext.hpp
    Nebula/VulkanRHI/DebugContext.cpp
    Nebula/VulkanRHI/Rendering/Shader.hpp
    Nebula/VulkanRHI/Rendering/Shader.cpp
    Nebula/VulkanRHI/Rendering/Pipeline.hpp
    Nebula/VulkanRHI/Rendering/Pipeline.cpp
    Nebula/VulkanRHI/Rendering/GraphicsPipeline.hpp
    Nebula/VulkanRHI/Rendering/GraphicsPipeline.cpp
    Nebula/VulkanRHI/Rendering/ComputePipeline.hpp
    Nebula/VulkanRHI/Rendering/ComputePipeline.cpp
    Nebula/VulkanRHI/Rendering.hpp
    Nebula/VulkanRHI/Rendering/RenderPass.hpp
    Nebula/VulkanRHI/Rendering/RenderPass.cpp
    Nebula/VulkanRHI/Commands.hpp
)

target_include_directories(Nebula3 PUBLIC
    Nebula
    External/json/include
    External/glfw/include
    External/glm
    External/vma/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME}
    ${Vulkan_LIBRARIES}
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator
)

target_compile_definitions(Nebula3 PUBLIC
    NBL_CONFIG_REGEN
    GLFW_INCLUDE_VULKAN
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_precompile_headers(Nebula3 PUBLIC
    Nebula/Impl/std_macos.pch.hpp
)
