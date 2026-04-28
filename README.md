## Nebula
A cross-platform rendering engine built with Vulkan.

<p align="center">
  <img src="./Resources/bistro_test.png" width="75%" />
</p>
<p align="center">Bistro Scene with RT Shadows</p>

<p align="center">
  <img src="./Resources/gi.png" width="75%" />
</p>
<p align="center">Single-bounce diffuse GI with Ray Queries</p>



### Features
- Supports Windows and macOS
- GPU-driven rendering
- Vulkan raytracing support
- User interface with ImGui
- GLTF loading
- Gamepad Support
- Basic Scene system
  - Slot pools with generational handles for data management
  - Geometry
    - Single Vertex and Index buffer
    - Bottom-Level AS management with a single backing buffer
  - Per-geometry batched instanced rendering
  - Textures
  - Materials
  - Top-Level AS Management
    - Compute-based data updates using the existing instance data
    - Used for GI, shadows, ambient occlusion and object selection
- Rendering techniques
  - Deferred shading
  - Screen-space and raytraced ambient occlusion
  - PBR Lighting
  - Raytraced shadows
  - Single-bounce GI : Indirect Diffuse Lighting
  - Anti-aliasing (FXAA)
  - Rayleigh-Mie Procedural Sky
    - Directional Light
  - Multiple lights, Runtime editing via UI
  - Frustum Culling
    - Debug Shader for AABB visualization
  - [WIP] Dual Kawase blur bloom for emissive materials.
  - [WIP] Pure Raytracing with shadows and reflections
- Toy voxel terrain generator for test scenes
