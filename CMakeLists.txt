cmake_minimum_required(VERSION 4.0)
project(Nebula3)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

if (APPLE)
    find_package(Freetype REQUIRED)
else ()
    add_compile_options(/arch:AVX2)
endif ()

# Shader Tools
include(Tools/nrt/cmake/nrt.cmake)
nrt_build()

include(cmake/fetchSlang.cmake)

add_subdirectory(External/fastgltf          EXCLUDE_FROM_ALL)
add_subdirectory(External/glm               EXCLUDE_FROM_ALL)
add_subdirectory(External/json              EXCLUDE_FROM_ALL)
add_subdirectory(External/vma               EXCLUDE_FROM_ALL)
add_subdirectory(External/fmt               EXCLUDE_FROM_ALL)
add_subdirectory(External/gemmi             EXCLUDE_FROM_ALL)
add_subdirectory(External/SDL               EXCLUDE_FROM_ALL)
add_subdirectory(External/SDL_ttf           EXCLUDE_FROM_ALL)
add_subdirectory(External/spdlog            EXCLUDE_FROM_ALL)
add_subdirectory(External/meshopt           EXCLUDE_FROM_ALL)

add_subdirectory(sunflower)

set("IMGUI_DIR" External/imgui)
set("IMGUIZMO_DIR" External/imguizmo)
set("IMGUI_FILES"
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp

    ${IMGUIZMO_DIR}/src/ImGuizmo.h ${IMGUIZMO_DIR}/src/ImGuizmo.cpp
)

set(IMNODES_DIR External/imnodes)
set(IMNODES_FILES ${IMNODES_DIR}/imnodes.cpp)

set(NBL_HAIR_FILES
    Nebula/Vendor/cy/cyCore.h
    Nebula/Vendor/cy/cyHairFile.h
    Nebula/Hair/CyLoader.cpp
    Nebula/Hair/CyLoader.hpp
    Nebula/Hair/HairData.hpp
    Nebula/Hair/HairGeometry.hpp
    Nebula/Hair/Utils.hpp
    Nebula/Hair/HairGeometry.cpp
    Nebula/Hair/Utils.cpp
    # Nebula/Hair/Render/ClassicHairRenderer.hpp
    # Nebula/Hair/Render/ClassicHairRenderer.cpp
    # Nebula/Hair/Render/HairRenderingMode.hpp
    # Nebula/Hair/Render/HairRenderer.hpp
    # Nebula/Hair/Render/HairRenderer.cpp
    # Nebula/Hair/Render/Complex/SoftwareRasterizer.hpp
    # Nebula/Hair/Render/Complex/SoftwareRasterizer.cpp
    # Nebula/Hair/Render/Complex/Shared.hpp
    # Nebula/Hair/Render/Complex/HybridHair_MeshStage.hpp
    # Nebula/Hair/Render/Complex/HybridHairRenderer.hpp
    # Nebula/Hair/Render/Complex/HybridHair_SoftwareStage.hpp
    # Nebula/Hair/Render/Complex/HybridHair_DebugCompose.hpp
    # Nebula/Hair/Render/IHairRenderer.hpp

    Nebula/Views/hair/HairView.cpp
    Nebula/Hair/Hybrid/MarschnerBSDF.hpp
    Nebula/Hair/Hybrid/GPUScreenSpaceTriangle.hpp
    Nebula/Hair/Hybrid/Shared.hpp
    Nebula/Hair/Hybrid/Shared.cpp
    Nebula/Hair/Hybrid/HairRenderer.hpp
    Nebula/Hair/Hybrid/HairRenderer.cpp
)

set(V2_FILES
    Nebula/Level/Camera/GpuCameraData.hpp
    Nebula/Level/Camera/Camera.hpp
    Nebula/Level/Camera/OrbitCamera.cpp
    Nebula/Level/Camera/OrbitCamera.hpp
    Nebula/Level/Camera/FlyingCamera.hpp
    Nebula/Level/Camera/FlyingCamera.cpp
    Nebula/Level/Camera/CameraSystem.hpp
    Nebula/Level/Level.hpp
    Nebula/Level/Camera/Camera.cpp
    Nebula/Level/Camera/CameraSystem.cpp
    Nebula/Level/Light/Light.hpp
    Nebula/Level/Light/Light.cpp
    Nebula/Level/Light/LightSystem.hpp
    Nebula/Level/Light/LightSystem.cpp
    Nebula/Level/Object/Object.hpp
    Nebula/Level/Object/SelectObjectFeature.hpp
    Nebula/Level/Geometry/Geometry.hpp
    Nebula/Level/Geometry/Vertex.hpp
    Nebula/Level/Geometry/Geometry.cpp
    Nebula/Level/Geometry/GeometrySystem.hpp
    Nebula/Level/Geometry/GeometryInfo.hpp
    Nebula/Level/Geometry/GeometrySystem.cpp
    Nebula/Level/Geometry/TangentGeneration.cpp
    Nebula/Level/Geometry/TangentGeneration.hpp
    Nebula/Level/Instance/InstanceSystem.hpp
    Nebula/Level/Transform.hpp
    Nebula/Level/Object/RotatingObject.hpp
    Nebula/Level/Render/LevelRenderer.hpp
    Nebula/Level/Level.cpp
    Nebula/Level/Raytracing/BLASSystem.hpp
    Nebula/Level/Raytracing/TLASSystem.hpp
    Nebula/Level/Raytracing/BLASSystem.cpp
    Nebula/Level/Raytracing/TLASSystem.cpp
    Nebula/Level/Render/Templates.hpp
    Nebula/Level/Render/BoundingBoxDebugPass.cpp
    Nebula/Level/Render/LevelRenderer.cpp
    Nebula/Level/Render/GBufferPass.hpp
    Nebula/Level/Render/GBufferPass.cpp
    Nebula/Level/Render/LightingPass.hpp
    Nebula/Level/Render/LightingPass.cpp
    Nebula/Level/Render/TonemapPass.hpp
    Nebula/Level/Render/TonemapPass.cpp
    Nebula/Level/Render/AntiAliasingPass.cpp
    Nebula/Level/Render/AntiAliasingPass.hpp
    Nebula/Level/GLTF/GLTFLoader.hpp
    Nebula/Level/GLTF/GLTFLoader.cpp
    Nebula/VulkanRHI/Render/Pipeline.hpp
    Nebula/Interface/Interface.hpp
    Nebula/Interface/Interface.cpp
    Nebula/VulkanRHI/RHIFeatures.hpp
    Nebula/VulkanRHI/RHIFeatures.cpp
    Nebula/VulkanRHI/Render/Pipeline.cpp
    Nebula/Interface/FadeEffect.hpp
    Nebula/Core/View.hpp
    Nebula/Views/LevelView.hpp
    Nebula/Views/hair/HairView.hpp
    Nebula/Views/UITestView.hpp
    Nebula/Core/View.cpp
    Nebula/Level/Render/BloomPass.hpp
    Nebula/Level/Render/BloomPass.cpp
    Nebula/Level/Voxel/VoxelData.hpp
    Nebula/Level/Voxel/IFeatureGenerator.hpp
    Nebula/Level/Voxel/TerrainGenerator.cpp
    Nebula/Level/Voxel/TerrainGenerator.hpp
    Nebula/Level/Voxel/Features/FoliageGenerator.cpp
    Nebula/Level/Voxel/Features/FoliageGenerator.hpp
    Nebula/Views/PathTracerView.hpp
    Nebula/Views/PathTracerView.cpp
    Nebula/Level/Light/DiscretePDF.hpp
    Nebula/Level/Light/DiscretePDF.cpp
    Nebula/Level/Light/AreaEmitter.hpp
    Nebula/VulkanRHI/Integration/DLSS.hpp
    Nebula/VulkanRHI/Integration/DLSS.cpp
    Nebula/Level/Render/PrePass.hpp
    Nebula/Level/Render/PrePass.cpp
    Nebula/Level/Object/SelectObjectFeature.cpp
    Nebula/ShaderBuild.hpp
    Nebula/ShaderBuild.cpp
    Nebula/Views/GameTest.hpp
    Nebula/Level/Camera/IsoCamera.hpp
    Nebula/Level/Render/SSAOPass.hpp
    Nebula/Level/Render/SSAOPass.cpp
    Nebula/Level/TextureSystem.hpp
    Nebula/Hair/Hybrid/Stages/Hybrid_MeshStage.hpp
    Nebula/Hair/Hybrid/Stages/Hybrid_MeshStage.cpp
    Nebula/Hair/Hybrid/Stages/Hybrid_SoftwareStage.cpp
    Nebula/RHI/DynamicRHI.hpp
    Nebula/RHI/Timeline.hpp
    Nebula/VulkanRHI/Commands/VulkanTimelineSync.hpp
    sunflower/main.cpp
)

add_executable(Nebula3
    ${IMGUI_FILES} ${IMNODES_FILES} ${V2_FILES} ${NBL_HAIR_FILES}
    External/mikktspace/mikktspace.c
    External/mikktspace/mikktspace.h

    Nebula/main.cpp

    Nebula/Impl/pch.hpp
    Nebula/Core/Configuration.hpp
    Nebula/Core/AppSpecification.hpp
    Nebula/Core/Types.hpp
    Nebula/Core/App.hpp
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
    Nebula/VulkanRHI/Render/Shader.hpp
    Nebula/VulkanRHI/Render/Shader.cpp
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
    Nebula/VulkanRHI/Raytracing/ShaderBindingTable.cpp
    Nebula/VulkanRHI/Raytracing/ShaderBindingTable.hpp
    Nebula/VulkanRHI/Raytracing.hpp
    Nebula/VulkanRHI/Allocation.hpp
    Nebula/Level/Vertex.hpp
    Nebula/Level/GPUObjectInstanceData.hpp
    Nebula/VulkanRHI/Render/VertexTraits.hpp
    Nebula/VulkanRHI/Detail/ImageTraits.hpp
    Nebula/UserInterface/ImGuiRenderPass.hpp
    Nebula/UserInterface/ImGuiRenderPass.cpp
    Nebula/VulkanRHI/Detail/BufferTraits.hpp
    Nebula/VulkanRHI/Detail/Resource.hpp
    Nebula/VulkanRHI/Detail/Resource.cpp
    Nebula/VulkanRHI/Allocation.cpp
    Nebula/VulkanRHI/Frame.cpp
    Nebula/VulkanRHI/Detail/DescriptorWrite.hpp
    Nebula/Level/TextureManager.hpp
    Nebula/Level/TextureManager.cpp
    Nebula/Core/Platform.hpp
    Nebula/Window/SDLWindow.hpp
    Nebula/Window/WindowCreateInfo.hpp
    Nebula/Window/SDLWindow.cpp
    Nebula/Core/Ranges.hpp
    Nebula/Math/HeightMap.hpp
    Nebula/Core/Random.hpp
    Nebula/Math/HeightMap.cpp
    Nebula/VulkanRHI/Common.hpp
    Nebula/VulkanRHI/Raytracing/AccelerationStructure.hpp
    Nebula/VulkanRHI/Raytracing/AccelerationStructure.cpp
    Nebula/Math/Transform.cpp
    Nebula/VulkanRHI/Detail/DescriptorWrite.cpp
    Nebula/Window/SplashWindow.hpp
    Nebula/Window/SplashWindow.cpp
    Nebula/VulkanRHI/Detail/Extensions.hpp
    Nebula/VulkanRHI/Detail/Extensions.cpp
    Nebula/VulkanRHI/Detail/DeviceExtensions.hpp
    Nebula/VulkanRHI/Detail/DeviceExtensions.cpp
    Nebula/Input/DualSense.hpp
    Nebula/Input/Gamepad.hpp
    Nebula/Input/Gamepad.cpp
    Nebula/Math/BoundingBox.hpp
    Nebula/Math/BoundingBox.cpp
    Nebula/Level/SlotPool.hpp
    Nebula/Level/Material/MaterialPool.hpp
    Nebula/Game/TitleScreen.hpp
    Nebula/Game/FontFace.hpp
    Nebula/Game/UIGeometry.hpp
    Nebula/Game/TitleScreen.cpp
)

target_include_directories(Nebula3 PUBLIC
    Nebula
    Nebula/Vendor/cy
    External/fastgltf/include
    External/freetype2
    External/glm
    External/imgui
    External/imgui/backends
    External/imgui/misc/cpp
    External/ImGuizmo/src
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
    External/xxhash
    ${Vulkan_INCLUDE_DIRS}
    External/DLSS/include
)

target_link_libraries(Nebula3 PUBLIC
    ${NLOHMANN_JSON_TARGET_NAME}
    ${Vulkan_LIBRARIES}
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    spdlog::spdlog
    fastgltf::fastgltf
    meshoptimizer
    slang::slang
)

if (APPLE)
    target_link_libraries(Nebula3 PUBLIC Freetype::Freetype)
else ()
    target_link_libraries(Nebula3 PUBLIC freetype)

    # DLSS
    # ================================================
    add_library(DLSS::NGX STATIC IMPORTED GLOBAL)

    set_target_properties(DLSS::NGX PROPERTIES
        IMPORTED_LOCATION             ${PROJECT_SOURCE_DIR}/External/DLSS/lib/Windows_x86_64/x64/nvsdk_ngx_d.lib
        IMPORTED_LOCATION_DEBUG       ${PROJECT_SOURCE_DIR}/External/DLSS/lib/Windows_x86_64/x64/nvsdk_ngx_d_dbg.lib
        INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/External/DLSS/include
    )

    target_link_libraries(Nebula3 PRIVATE DLSS::NGX)
    target_compile_definitions(Nebula3 PRIVATE nbl_DLSS_AVAILABLE)
endif ()

target_compile_definitions(Nebula3 PUBLIC
    NBL_CONFIG_REGEN
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    -DImTextureID=ImU64
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    nbl_MOLTEN_VK
)

target_precompile_headers(Nebula3 PUBLIC
    Nebula/Impl/pch.hpp
)
set_source_files_properties(External/mikktspace/mikktspace.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
