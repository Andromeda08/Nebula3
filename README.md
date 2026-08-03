## Nebula
A cross-platform rendering engine built with Vulkan.

<p align="center">
  <img src="./Resources/pt.png" width="75%" />
</p>
<p align="center">Ray Tracing Pipeline MIS Path Tracer</p>

<p align="center">
  <img src="./Resources/hair_preview.png" width="75%" />
</p>
<p align="center">[Preview] GPU-driven hybrid hair rendering (mesh shader, compute rasterizer)</p>

<p align="center">
  <img src="./Resources/bistro_test.png" width="75%" />
</p>
<p align="center">Bistro Scene with RT Shadows</p>

<table align="center">
   <tr>
      <td align="center">
         <img src="./Resources/gi.png" width="85%" />
      </td>
      <td align="center">
         <img src="./Resources/molecule.png" width="75%" />
      </td>
   </tr>
   <tr>
      <td align="center">
         <p>Experimental Single-bounce GI with RQ</p>
      </td>
      <td align="center">
         <p>Impl. of <a href="https://www.uni-ulm.de/fileadmin/website_uni_ulm/iui.inst.100/institut/Papers/viscom/2018/hermosilla2018molsss.pdf">Hermosilla et al. 2018</a> with <a href="https://github.com/peterkain">@peterkain</a></p>
      </td>
   </tr>
</table>

<p align="center">

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
  - Texture System
  - Materials System
  - Top-Level AS Management
    - Compute-based data updates using the existing instance data
    - Used for PT, GI, shadows, ambient occlusion and object selection
- Experimental voxel terrain generator for test scenes
- Rendering techniques
  - Ray tracing pipeline-based MIS path tracer with support for any BSDF. 
    - Current BSDFs: Diffuse, Mirror, Dielectric
    - Mesh Emitters
    - Temporal accumulation for stationary camera (a)
    - DLSS Ray Reconstruction integration (b)
  - Deferred shading
  - Screen-space and raytraced ambient occlusion
  - Raytraced shadows
  - PBR Lighting
  - Anti-aliasing (FXAA)
  - Procedural Sky
  - Multiple lights, Runtime editing via UI
  - Frustum Culling
    - Debug Shader for AABB visualization
  - Tone Mapping (currently AgX)
  - Single-bounce GI Experiment → Indirect Diffuse Lighting
