cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

# Shader Tools
include(Tools/nrt/cmake/nrt.cmake)
nrt_build()

add_subdirectory(External/fastgltf  EXCLUDE_FROM_ALL)
add_subdirectory(External/glm       EXCLUDE_FROM_ALL)
add_subdirectory(External/json      EXCLUDE_FROM_ALL)
add_subdirectory(External/vma       EXCLUDE_FROM_ALL)
add_subdirectory(External/gemmi     EXCLUDE_FROM_ALL)
add_subdirectory(External/SDL       EXCLUDE_FROM_ALL)
add_subdirectory(External/SDL_ttf   EXCLUDE_FROM_ALL)
add_subdirectory(External/spdlog    EXCLUDE_FROM_ALL)
add_subdirectory(External/meshopt   EXCLUDE_FROM_ALL)

add_compile_options(/arch:AVX2)

set("IMGUI_DIR" External/imgui)
set("IMGUI_FILES"
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(IMNODES_DIR External/imnodes)
set(IMNODES_FILES ${IMNODES_DIR}/imnodes.cpp)

set(Nebula_RenderGraph_Sources
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
    Nebula/RenderGraph/Compiler/RenderGraphCompiler.hpp
    Nebula/RenderGraph/Compiler/GraphAlgorithm.hpp
    Nebula/RenderGraph/Compiler/ResourceOptimizer.hpp
    Nebula/RenderGraph/Compiler/ResourceOptimizer.cpp
    Nebula/RenderGraph/RenderPath/Resource.hpp
    Nebula/RenderGraph/Compiler/RenderGraphCompiler.cpp
    Nebula/RenderGraph/Compiler/GraphAlgorithm.cpp
    Nebula/RenderGraph/RenderPath/RenderPath.hpp
    Nebula/RenderGraph/RenderPath/RenderPath.cpp
    Nebula/RenderGraph/RenderPath/Resource.cpp
    Nebula/RenderGraph/RenderPath/PassFactory.hpp
    Nebula/RenderGraph/RenderPath/PassFactory.cpp
    Nebula/RenderGraph/DependencyInfo.cpp
    Nebula/RenderGraph/Builder/RenderGraphBuilder.hpp
    Nebula/RenderGraph/Builder/RenderGraphBuilder.cpp
    Nebula/RenderGraph/Compiler/Export.hpp
    Nebula/RenderGraph/Compiler/Export.cpp
    Nebula/RenderGraph/RenderPath/Resources/Texture2DResource.hpp
    Nebula/RenderGraph/RenderPath/Resources/Texture3DResource.hpp
    Nebula/RenderGraph/RenderPath/Resources/ResourceRegistry.hpp
    Nebula/RenderGraph/RenderPath/Resources/SceneResource.hpp
    Nebula/RenderGraph/RenderPath/Resources/ResourceRegistry.cpp
)

set(Nebula_Molecule_Sources
    Nebula/Scene/Molecule/CIFData.hpp
    Nebula/Scene/Molecule/CIFData.cpp
    Nebula/Scene/Molecule/CIFParser.cpp
    Nebula/Scene/Molecule/CIFParser.hpp
    Nebula/Scene/Molecule/CIFGeometry.hpp
    Nebula/RenderPass/Molecule/SDFComputePass.cpp
    Nebula/RenderPass/Molecule/StructurePass.hpp
    Nebula/RenderPass/Molecule/StructurePass.cpp
    Nebula/RenderPass/Molecule/SDFRaymarchPass.hpp
    Nebula/RenderPass/Molecule/SDFRaymarchPass.cpp
    Nebula/Scene/Scenes/MoleculeScene/MoleculeSceneParamsComponent.hpp
    Nebula/Scene/Scenes/MoleculeScene/MoleculeSceneParamsComponent.cpp
    Nebula/Scene/Molecule/RenderingOptions.hpp
    Nebula/RenderPass/Molecule/SDFComputePass.hpp
    Nebula/Scene/Molecule/CIFGeometry.cpp
    Nebula/Scene/Scenes/MoleculeScene/MoleculeScene.hpp
    Nebula/Scene/Scenes/MoleculeScene/MoleculeScene.cpp
)

add_executable(Nebula3
    ${IMGUI_FILES} ${IMNODES_FILES}
    External/mikktspace/mikktspace.c
    External/mikktspace/mikktspace.h

    Nebula/Impl/pch.hpp
    Nebula/Core/Configuration.hpp
    Nebula/Core/AppSpecification.hpp
    Nebula/Core/Types.hpp
    Nebula/Core/App.hpp
    Nebula/main.cpp
    Nebula/Core/Configuration.cpp
    Nebula/Core/Macro.hpp
    Nebula/Core/App.cpp
    Nebula/Math/DeltaTime.hpp
    Nebula/Math/DeltaTime.cpp
    Nebula/VulkanRHI/IWindow.hpp
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
    Nebula/RenderPass/Special/ScenePass.hpp
    Nebula/RenderPass/Pass.hpp
    Nebula/RenderPass/Special/LightingPass.hpp
    Nebula/RenderPass/Special/AmbientOcclusionPass.hpp
    Nebula/RenderPass/Special/AntiAliasingPass.hpp
    Nebula/RenderPass/Special/PresentPass.hpp
    Nebula/VulkanRHI/Allocation.hpp
    Nebula/RenderPass/Special/Combine.hpp
    Nebula/RenderPass/RenderPass.hpp
    Nebula/Scene/Types/Vertex.hpp
    Nebula/Scene/Types/GPUObjectInstanceData.hpp
    Nebula/VulkanRHI/Rendering/VertexTraits.hpp
    Nebula/Scene/Scene.hpp
    Nebula/Scene/Types/Light.hpp
    Nebula/Scene/Scene.cpp
    Nebula/Scene/Components/SceneInfoComponent.hpp
    Nebula/Scene/Components/SceneInfoComponent.cpp
    Nebula/Scene/Geometry/Geometry.hpp
    Nebula/Scene/Geometry/Geometry.cpp
    Nebula/Scene/Types/CameraData.hpp
    Nebula/Scene/Camera/ICamera.hpp
    Nebula/Scene/Object.hpp
    Nebula/Scene/Camera/FlyingCamera.hpp
    Nebula/VulkanRHI/Detail/ImageTraits.hpp
    Nebula/UserInterface/ImGuiRenderPass.hpp
    Nebula/UserInterface/ImGuiRenderPass.cpp
    Nebula/VulkanRHI/Detail/BufferTraits.hpp
    Nebula/VulkanRHI/Detail/Resource.hpp
    Nebula/VulkanRHI/Detail/Resource.cpp
    Nebula/VulkanRHI/Allocation.cpp
    Nebula/VulkanRHI/Frame.cpp
    Nebula/VulkanRHI/Detail/DescriptorWrite.hpp
    Nebula/Scene/TextureManager.hpp
    Nebula/Scene/TextureManager.cpp
    Nebula/VulkanRHI/Texture.hpp
    Nebula/VulkanRHI/Texture.cpp
    Nebula/Core/Platform.hpp
    Nebula/RenderPass/Special/GBufferPass.hpp
    Nebula/Window/SDLWindow.hpp
    Nebula/Window/WindowCreateInfo.hpp
    Nebula/Window/SDLWindow.cpp
    Nebula/Scene/Camera/FlyingCamera.cpp
    Nebula/Core/Ranges.hpp
    Nebula/Math/HeightMap.hpp
    Nebula/Core/Random.hpp
    Nebula/Math/HeightMap.cpp
    Nebula/Scene/Voxel/TerrainGenerator.hpp
    Nebula/Scene/Voxel/TerrainGenerator.cpp
    Nebula/Scene/Voxel/VoxelScene.hpp
    Nebula/Scene/Voxel/GPUVoxelInstanceData.hpp
    Nebula/Scene/Voxel/Features/FoliageGenerator.hpp
    Nebula/Scene/Voxel/IFeatureGenerator.hpp
    Nebula/Scene/Voxel/VoxelData.hpp
    Nebula/Scene/Voxel/Features/FoliageGenerator.cpp
    Nebula/Scene/Voxel/Features/PillarGenerator.hpp
    Nebula/Scene/Types/Light.hpp
    Nebula/Scene/LightSystem.hpp
    Nebula/Scene/LightSystem.cpp
    Nebula/Scene/Voxel/vxlRenderPath.hpp
    Nebula/Scene/Voxel/vxlRenderPath.cpp
    Nebula/Scene/Voxel/VoxelScene.cpp
    Nebula/Scene/Render/SSAOPass.hpp
    Nebula/Scene/Render/SSAOPass.cpp
    Nebula/Scene/Render/Voxel_GBufferPass.hpp
    Nebula/Scene/Render/Voxel_GBufferPass.cpp
    Nebula/Scene/Render/LightingPass.hpp
    Nebula/Scene/Render/LightingPass.cpp
    Nebula/Scene/Render/RenderPass.hpp
    Nebula/Scene/Render/FXAAPass.hpp
    Nebula/Scene/Render/FXAAPass.cpp
    Nebula/VulkanRHI/Common.hpp
    Nebula/Scene/SceneV2.hpp
    Nebula/Scene/SceneGeometry.hpp
    Nebula/Scene/SceneGeometry.cpp
    Nebula/VulkanRHI/Raytracing/AccelerationStructure.hpp
    Nebula/VulkanRHI/Raytracing/AccelerationStructure.cpp
    Nebula/Scene/InstancePool.hpp
    Nebula/Scene/InstancePool.cpp
    Nebula/Math/Transform.cpp
    Nebula/Scene/TLASManager.hpp
    Nebula/Scene/TLASManager.cpp
    Nebula/Scene/Render/Indirect_GBufferPass.hpp
    Nebula/Scene/Render/Indirect_GBufferPass.cpp
    Nebula/VulkanRHI/Detail/DescriptorWrite.cpp
    Nebula/Scene/Render/RTAOPass.hpp
    Nebula/Scene/Render/RTAOPass.cpp
    Nebula/Scene/SceneV2.cpp
    Nebula/Window/SplashWindow.hpp
    Nebula/Window/SplashWindow.cpp
    Nebula/Scene/Render/ProceduralSky.hpp
    Nebula/Scene/Render/ProceduralSky.cpp
    Nebula/VulkanRHI/Detail/Extensions.hpp
    Nebula/VulkanRHI/Detail/Extensions.cpp
    Nebula/VulkanRHI/Detail/DeviceExtensions.hpp
    Nebula/VulkanRHI/Detail/DeviceExtensions.cpp
    Nebula/Scene/Render/TonemapPass.hpp
    Nebula/Scene/Render/TonemapPass.cpp
    Nebula/Scene/GLTF/GLTFLoader.hpp
    Nebula/Scene/GLTF/GLTFLoader.cpp
    Nebula/Input/DualSense.hpp
    Nebula/Input/Gamepad.hpp
    Nebula/Scene/Render/FullRT.hpp
    Nebula/Scene/Render/FullRT.cpp
    Nebula/Input/Gamepad.cpp
    Nebula/Math/BoundingBox.hpp
    Nebula/Math/vec.hpp
    Nebula/Scene/Render/AABBOverlayPass.cpp
    Nebula/Scene/Render/AABBOverlayPass.hpp
    Nebula/Cleanup/Scene.hpp
    Nebula/Cleanup/Geometry/GeometrySystem.hpp
    Nebula/Cleanup/VertexTypes.hpp
    Nebula/Cleanup/VertexTypes.cpp
    Nebula/Cleanup/Math/BoundingBox.hpp
    Nebula/Cleanup/Math/BoundingBox.cpp
    Nebula/Cleanup/Geometry/Geometry.hpp
    Nebula/Cleanup/Geometry/Meshlet.hpp
    Nebula/Cleanup/Geometry/Geometry.cpp
    Nebula/Cleanup/SceneManager.hpp
    Nebula/Cleanup/IO/SceneLoader.hpp
    Nebula/Cleanup/SceneManager.cpp
    Nebula/Cleanup/IO/SceneLoader.cpp
    Nebula/Cleanup/Geometry/TangentGeneration.hpp
    Nebula/Cleanup/Geometry/TangentGeneration.cpp
    Nebula/Cleanup/Timer.hpp
    Nebula/Cleanup/Timer.cpp
    Nebula/Cleanup/Math/Transform.hpp
    Nebula/Cleanup/Math/Transform.cpp
    Nebula/Cleanup/Object.hpp
    Nebula/Cleanup/Material/Material.hpp
    Nebula/Cleanup/Material/MaterialSystem.hpp
    Nebula/Cleanup/Instance/InstancePool.hpp
    Nebula/Cleanup/Geometry/GeometrySystem.cpp
    Nebula/Cleanup/Geometry/GeometrySystemDebugRenderPass.hpp
)

target_include_directories(Nebula3 PUBLIC
    Nebula
    External/fastgltf/include
    External/glm
    External/imgui
    External/imgui/backends
    External/imgui/misc/cpp
    External/imnodes
    External/json/include
    External/meshopt/src
    External/mikktspace
    External/stb
    External/vma/include
    External/gemmi/include
    External/SDL/include
    External/SDL_ttf/include
    External/spdlog/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME}
    ${Vulkan_LIBRARIES}
    glm::glm
    # gemmi::gemmi_cpp
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    spdlog::spdlog
    fastgltf::fastgltf
    meshoptimizer
)

target_compile_definitions(Nebula3 PUBLIC
    NBL_CONFIG_REGEN
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    -DImTextureID=ImU64
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_precompile_headers(Nebula3 PUBLIC
    Nebula/Impl/pch.hpp
)
set_source_files_properties(External/mikktspace/mikktspace.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
