cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

add_subdirectory(External/glfw)
add_subdirectory(External/json)

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
    Nebula/Core/ToString.hpp)

target_include_directories(Nebula3 PUBLIC
    Nebula
    External/json/include
    External/glfw/include
    ${Vulkan_INCLUDE_DIRS})

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME}
    ${Vulkan_LIBRARIES}
    glfw)

target_compile_definitions(Nebula3 PUBLIC
    GLFW_INCLUDE_VULKAN)