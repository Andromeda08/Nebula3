#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "nbl/Camera.inc.glsl"
#include "nbl/Instance.inc.glsl"

layout (push_constant) uniform PushConstants {
    vec4                boxColor;
    InstanceBuffer      instances;
    CameraBuffer        camera;
    uint                instanceGpuIndex;
};

// Edges for the bounding box
const ivec2 edges[12] = ivec2[](
    ivec2(0,1), ivec2(1,3), ivec2(3,2), ivec2(2,0),
    ivec2(4,5), ivec2(5,7), ivec2(7,6), ivec2(6,4),
    ivec2(0,4), ivec2(1,5), ivec2(3,7), ivec2(2,6)
);

// Get position for current edge & vertex.
vec3 getPosition(const vec3 aabbMin, const vec3 aabbMax)
{
    const int edgeIndex   = gl_VertexIndex / 2;
    const int vertexIndex = (gl_VertexIndex % 2 == 0) ? edges[edgeIndex].x : edges[edgeIndex].y;

    const vec3 a = vec3(
        (vertexIndex & 1) != 0,
        (vertexIndex & 2) != 0,
        (vertexIndex & 4) != 0
    );

    return mix(aabbMin, aabbMax, a);
}

void main()
{
    GPUInstanceData instanceData = instances.data[instanceGpuIndex];

    vec3 pos = getPosition(instanceData.aabbMin.xyz, instanceData.aabbMax.xyz);

    gl_Position = camera.data.proj * camera.data.view * vec4(pos, 1.0);
}
