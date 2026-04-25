#version 460

#extension GL_EXT_ray_query : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "inc/nbl.inc.glsl"

layout (local_size_x = 1) in;

layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout (buffer_reference, scalar) readonly buffer CameraDataBuffer {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) writeonly buffer ResultBuffer {
    int objectID;
};

layout (push_constant) uniform PushConstants {
    CameraDataBuffer    camera;
    InstanceBuffer      instanceBuffer;
    ResultBuffer        resultBuffer;
    vec2                mousePos;
    vec2                screenSize;
};

void main()
{
    vec2 ndcMousePos = vec2(
        (2.0 * mousePos.x) / screenSize.x - 1.0,
        (2.0 * mousePos.y) / screenSize.y - 1.0
    );
    vec4 farView = camera.projInverse * vec4(ndcMousePos, 1.0, 1.0);
    farView /= farView.w;
    vec3 farWorld = (camera.viewInverse * vec4(farView.xyz, 1.0)).xyz;

    vec3 origin = camera.eye.xyz;
    vec3 dir    = normalize(farWorld - origin);

    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, origin, camera.nearPlane, dir, camera.farPlane);

    while (rayQueryProceedEXT(rq)) {}

    if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        uint idx = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);

        GPUInstanceData instance = instanceBuffer.instances[idx];
        resultBuffer.objectID = instance.objectId;
    }
    else
    {
        resultBuffer.objectID = -1;
    }
}
