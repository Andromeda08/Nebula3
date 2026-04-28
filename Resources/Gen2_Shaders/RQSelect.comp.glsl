#version 460

#extension GL_EXT_ray_query : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#include "nbl/Camera.inc.glsl"
#include "nbl/Instance.inc.glsl"

#define nbl_RT
#define nbl_TLAS_SET 0
#include "nbl/TLAS.inc.glsl"

layout (local_size_x = 1) in;

layout (buffer_reference, std430) writeonly buffer ResultBuffer {
    int objectID;
};

layout (push_constant) uniform PushConstants {
    InstanceBuffer      instances;
    CameraBuffer        camera;
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
    vec4 farView = camera.data.projInverse * vec4(ndcMousePos, 1.0, 1.0);
    farView /= farView.w;
    vec3 farWorld = (camera.data.viewInverse * vec4(farView.xyz, 1.0)).xyz;

    vec3 origin = camera.data.eye.xyz;
    vec3 dir    = normalize(farWorld - origin);

    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, origin, camera.data.nearPlane, dir, camera.data.farPlane);

    while (rayQueryProceedEXT(rq)) {}

    if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        uint idx = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);

        GPUInstanceData instance = instances.data[idx];
        resultBuffer.objectID = instance.objectId;
    }
    else
    {
        resultBuffer.objectID = -1;
    }
}
