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
- [wip] RenderGraph system with a node editor and support for serialization
- GLTF loading
- Basic Scene system
  - Geometry
    - Single Vertex and Index buffer
    - Bottom-Level AS management with a single backing buffer
  - Instanced rendering
  - Textures
  - Top-Level AS Management
    - Compute-based data updates using the existing instance data
- Toy voxel terrain generator for test scenes
- Rendering techniques
  - Deferred shading
  - Screen-space and raytraced ambient occlusion
  - Raytraced shadows
  - Anti-aliasing (FXAA)
  - Rayleigh-Mie Procedural Sky
  - Molecule rendering (CIF files via instanced rendering and ray marching)
- WIP implementations of features from old engine versions
  - Pure Raytracing with shadows and reflections
  - GPU-Driven hair rendering with mesh shaders
