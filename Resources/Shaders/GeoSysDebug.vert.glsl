#version 460

layout(location = 0) in vec3 inPosition;

layout (set = 0, binding = 0) uniform CameraData {
    mat4    view;
    mat4    proj;
    mat4    viewInverse;
    mat4    projInverse;
    vec4    eye;
    vec4    frustumPlanes[6];
    float   nearPlane;
    float   farPlane;
} camera;

layout(push_constant) uniform PushConstant {
    mat4 model;
    int  geometryIndex;
};

void main()
{
    gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
}