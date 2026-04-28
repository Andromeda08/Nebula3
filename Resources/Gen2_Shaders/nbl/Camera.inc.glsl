#ifndef nbl_CAMERA_INC_GLSL
#define nbl_CAMERA_INC_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

// Camera Data
// Declares a [buffer_reference] with type name "CameraBuffer"
// ------------------------------------------------------------
// Camera data from the previous frame is not always required,
// to declare the buffer reference define "nbl_CAMERA_HISTORY"
// before including this file.
// Declares a [buffer_reference] with type name "HistoryCameraBuffer"
// ============================================================

struct GPUCameraData
{
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

// Get the linear depth (view Z) for a depth value in [0, 1] for a perspective projection.
float getLinearDepth(GPUCameraData camera, float depth)
{
    return (camera.nearPlane * camera.farPlane) /
        (camera.farPlane - depth * (camera.farPlane - camera.nearPlane));
}

// View to World Space conversion for points.
vec3 viewToWorld_Point(vec3 viewPosition, GPUCameraData camera)
{
    return (camera.viewInverse * vec4(viewPosition, 1.0)).xyz;
}

// View to World Space conversion for vectors.
vec3 viewToWorld_Vector(vec3 viewDirection, GPUCameraData camera)
{
    return mat3(camera.viewInverse) * viewDirection;
}

// World to View Space conversion for points.
vec3 worldToView_Point(vec3 worldPosition, GPUCameraData camera)
{
    return (camera.view * vec4(worldPosition, 1.0)).xyz;
}

// World to View Space conversion for vectors.
vec3 worldToView_Vector(vec3 worldDirection, GPUCameraData camera)
{
    return mat3(camera.view) * worldDirection;
}

// [CameraBuffer] Buffer Reference
layout (buffer_reference, scalar) readonly buffer CameraBuffer
{
    GPUCameraData data;
};

// [HistoryCameraBuffer] Buffer Reference
#ifdef nbl_CAMERA_HISTORY
layout (buffer_reference, scalar) readonly buffer HistoryCameraBuffer
{
    GPUCameraData data;
};
#endif

#endif
