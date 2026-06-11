#version 460

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    mat4  proj;
    vec4  color;
    int   textureIndex;
    int   isText;
};

layout(location = 0) out vec2 vUV;

void main()
{
    gl_Position = proj * vec4(inPos, 0.0, 1.0);
    vUV = inUV;
}
