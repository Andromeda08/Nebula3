#version 460

#extension GL_GOOGLE_include_directive : enable

#include "inc/Nebula.inc.glsl"
#include "inc/Vulkan.inc.glsl"

layout (push_constant) uniform PushConstant {
    uint64_t instanceBufferAddress;
    uint64_t instanceMapAddress;
};

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) readonly buffer InstanceMapBuffer {
    uint indices[];
};

layout (buffer_reference, std430) readonly buffer IndirectDrawBuffer {
    vk_DrawIndexedIndirectCommand indirectDraws[];
};

layout (set = 0, binding = 0) uniform CameraData camera;
