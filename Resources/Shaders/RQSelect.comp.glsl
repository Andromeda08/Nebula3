#version 460

#extension GL_EXT_ray_query : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (local_size_x = 1) in;

struct GPUInstanceData
{
    mat4      model;
    vec4      boundsMin;
    vec4      boundsMax;
    uint64_t  blasAddress;
    int       materialIndex;
    int       geometryIndex;
    int       objectID;
    int       _pad0;
    int       _pad1;
    int       _pad2;
};

layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
} camera;

layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

layout (buffer_reference, std430) writeonly buffer ResultBuffer {
    int objectID;
};

layout (buffer_reference, std430) readonly buffer InstanceBuffer {
    GPUInstanceData instances[];
};

layout (buffer_reference, std430) readonly buffer InstanceMapBuffer {
    uint indices[];
};

layout (push_constant) uniform PushConstants {
    uint64_t instanceBufferAddress;
    uint64_t resultBufferAddress;
    vec2     mousePos;
    vec2     screenSize;
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

    ResultBuffer resultBuffer = ResultBuffer(resultBufferAddress);
    if (rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        uint idx = rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true);

        InstanceBuffer  instanceBuffer = InstanceBuffer(instanceBufferAddress);
        GPUInstanceData instance       = instanceBuffer.instances[idx];


        resultBuffer.objectID = instance.objectID;
    }
    else
    {
        resultBuffer.objectID = -1;
    }
}
