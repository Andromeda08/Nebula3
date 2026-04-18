## Nebula
A cross-platform rendering engine built with Vulkan.

<p align="center">
  <img src="./Resources/bistro_test.png" width="75%" />
</p>


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
  - Instanced rendering
  - Textures
  - Materials
  - Top-Level AS Management
    - Compute-based data updates using the existing instance data
- Rendering techniques
  - Deferred shading
  - Screen-space and raytraced ambient occlusion
  - PBR Lighting
  - Raytraced shadows
  - Anti-aliasing (FXAA)
  - Rayleigh-Mie Procedural Sky
  - Frustum Culling
    - Debug Shader for AABB visualization
  - [WIP] Dual Kawase blur bloom for emissive materials.
  - [WIP] Pure Raytracing with shadows and reflections
- Toy voxel terrain generator for test scenes
