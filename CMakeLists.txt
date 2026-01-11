cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

# Shader Tools
include(Tools/nrt/cmake/nrt.cmake)
nrt_build()

add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/json)
add_subdirectory(External/vma)
add_subdirectory(External/gemmi EXCLUDE_FROM_ALL)

set("IMGUI_DIR" External/imgui)
set("IMGUI_FILES"
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(IMNODES_DIR External/imnodes)
set(IMNODES_FILES ${IMNODES_DIR}/imnodes.cpp)

add_executable(Nebula3
    ${IMGUI_FILES} ${IMNODES_FILES}
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
    Nebula/Impl/stb.impl.cpp
    Nebula/Impl/std_macos.pch.hpp
    Nebula/Impl/vma.impl.cpp
    Nebula/VulkanRHI/Device.cpp
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
    Nebula/VulkanRHI/Rendering/ComputePipeline.hpp
    Nebula/VulkanRHI/Rendering/ComputePipeline.cpp
    Nebula/VulkanRHI/Rendering/GraphicsPipeline.hpp
    Nebula/VulkanRHI/Rendering/GraphicsPipeline.cpp
    Nebula/VulkanRHI/Rendering/Pipeline.hpp
    Nebula/VulkanRHI/Rendering/Pipeline.cpp
    Nebula/VulkanRHI/Rendering/Shader.hpp
    Nebula/VulkanRHI/Rendering/Shader.cpp
    Nebula/VulkanRHI/Rendering.hpp
    Nebula/VulkanRHI/Rendering/RenderPass.hpp
    Nebula/VulkanRHI/Rendering/RenderPass.cpp
    Nebula/VulkanRHI/Commands/CommandList.hpp
    Nebula/VulkanRHI/Commands/CommandList.cpp
    Nebula/VulkanRHI/Commands/CommandPool.hpp
    Nebula/VulkanRHI/Commands/CommandPool.cpp
    Nebula/VulkanRHI/Commands/CommandQueue.hpp
    Nebula/VulkanRHI/Commands/CommandQueue.cpp
    Nebula/VulkanRHI/Commands.hpp
    Nebula/UserInterface/UserInterface.cpp
    Nebula/UserInterface/UserInterface.hpp
    Nebula/UserInterface/IComponent.hpp
    Nebula/UserInterface/Components/StatisticsComponent.hpp
    Nebula/UserInterface/Components/StatisticsComponent.cpp
    Nebula/VulkanRHI/Barrier.hpp
    Nebula/VulkanRHI/Barrier.cpp
    Nebula/VulkanRHI/Descriptor.cpp
    Nebula/VulkanRHI/Descriptor.hpp
    Nebula/VulkanRHI/Raytracing/RaytracingPipeline.cpp
    Nebula/VulkanRHI/Raytracing/RaytracingPipeline.hpp
    Nebula/VulkanRHI/Raytracing/ShaderBindingTable.cpp
    Nebula/VulkanRHI/Raytracing/ShaderBindingTable.hpp
    Nebula/VulkanRHI/Raytracing.hpp
    Nebula/RenderPass/HelloTrianglePass.hpp
    Nebula/RenderGraph/RenderGraph.hpp
    Nebula/RenderGraph/RGConfiguration.hpp
    Nebula/RenderGraph/RenderGraphTraits.hpp
    Nebula/RenderGraph/RenderGraph.cpp
    Nebula/RenderGraph/Node.hpp
    Nebula/RenderGraph/Node.cpp
    Nebula/RenderGraph/DependencyInfo.hpp
    Nebula/RenderGraph/RenderGraphContext.hpp
    Nebula/RenderGraph/Editor/RenderGraphEditorComponent.hpp
    Nebula/RenderGraph/Editor/RenderGraphEditorComponent.cpp
    Nebula/RenderGraph/RenderGraphContext.cpp
    Nebula/RenderGraph/RGConfiguration.cpp
    Nebula/RenderPass/Special/ScenePass.hpp
    Nebula/RenderPass/IPass.hpp
    Nebula/RenderGraph/Compiler/RenderGraphCompiler.hpp
    Nebula/RenderGraph/Compiler/GraphAlgorithm.hpp
    Nebula/RenderGraph/Compiler/ResourceOptimizer.hpp
    Nebula/RenderGraph/Compiler/ResourceOptimizer.cpp
    Nebula/RenderGraph/RenderPath/Resource.hpp
    Nebula/RenderPass/GBufferPass.hpp
    Nebula/RenderPass/Special/LightingPass.hpp
    Nebula/RenderPass/Special/AmbientOcclusionPass.hpp
    Nebula/RenderPass/Special/AntiAliasingPass.hpp
    Nebula/RenderPass/Special/PresentPass.hpp
    Nebula/RenderGraph/Compiler/RenderGraphCompiler.cpp
    Nebula/RenderGraph/Compiler/GraphAlgorithm.cpp
    Nebula/RenderGraph/RenderPath/RenderPath.hpp
    Nebula/RenderGraph/RenderPath/RenderPath.cpp
    Nebula/RenderGraph/RenderPath/Resource.cpp
    Nebula/RenderGraph/RenderPath/PassFactory.hpp
    Nebula/RenderGraph/RenderPath/PassFactory.cpp
    Nebula/RenderGraph/DependencyInfo.cpp
    Nebula/VulkanRHI/Allocation.hpp
    Nebula/RenderGraph/Builder/RenderGraphBuilder.hpp
    Nebula/RenderGraph/Builder/RenderGraphBuilder.cpp
    Nebula/RenderGraph/Compiler/Export.hpp
    Nebula/RenderGraph/Compiler/Export.cpp
    Nebula/RenderPass/Special/Combine.hpp
    Nebula/RenderPass/RenderPass.hpp
    Nebula/RenderPass/GBufferPass.cpp
    Nebula/Scene/Types/Vertex.hpp
    Nebula/Scene/Types/GPUObjectInstanceData.hpp
    Nebula/VulkanRHI/Rendering/VertexTraits.hpp
    Nebula/Scene/Scene.hpp
    Nebula/Scene/Types/LightParameters.hpp
    Nebula/Scene/SceneConfiguration.hpp
    Nebula/Scene/Scene.cpp
    Nebula/Scene/Components/SceneInfoComponent.hpp
    Nebula/Scene/Components/SceneInfoComponent.cpp
    Nebula/Scene/Geometry/Geometry.hpp
    Nebula/Scene/Geometry/Geometry.cpp
    Nebula/Scene/Types/CameraData.hpp
    Nebula/Scene/Camera/ICamera.hpp
    Nebula/Scene/Molecule/CIFData.hpp
    Nebula/Scene/Molecule/CIFData.cpp
    Nebula/Scene/Molecule/CIFParser.cpp
    Nebula/Scene/Molecule/CIFParser.hpp
    Nebula/Scene/Molecule/CIFGeometry.hpp
    Nebula/Scene/Camera/OrbitCamera.hpp
    Nebula/Scene/Object.hpp
    Nebula/Scene/Camera/FlyingCamera.hpp
    Nebula/VulkanRHI/Detail/ImageTraits.hpp
    Nebula/UserInterface/ImGuiRenderPass.hpp
    Nebula/VulkanRHI/Image3D.hpp
    Nebula/UserInterface/ImGuiRenderPass.cpp
    Nebula/VulkanRHI/Detail/BufferTraits.hpp
    Nebula/VulkanRHI/Detail/DescriptorWrite.hpp
    Nebula/RenderPass/Molecule/ComputePrePass.hpp
    Nebula/Scene/Molecule/CIFGeometry.cpp
    Nebula/VulkanRHI/Image3D.cpp
    Nebula/VulkanRHI/Detail/Resource.hpp
    Nebula/VulkanRHI/Detail/Resource.cpp
    Nebula/VulkanRHI/Allocation.cpp
)

target_include_directories(Nebula3 PUBLIC
    Nebula
    External/glfw/include
    External/glm
    External/imgui
    External/imgui/backends
    External/imgui/misc/cpp
    External/imnodes
    External/json/include
    External/stb
    External/vma/include
    External/gemmi/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME}
    ${Vulkan_LIBRARIES}
    glfw
    glm::glm
    gemmi::gemmi_cpp
    GPUOpen::VulkanMemoryAllocator
)

target_compile_definitions(Nebula3 PUBLIC
    NBL_CONFIG_REGEN
    GLFW_INCLUDE_VULKAN
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    -DImTextureID=ImU64
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_precompile_headers(Nebula3 PUBLIC
    Nebula/Impl/std_macos.pch.hpp
)
