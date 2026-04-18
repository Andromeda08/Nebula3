#version 460

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstant {
    mat4 model;
    int  geometryIndex;
};

vec3 indexToColor(uint i)
{
    uint h = i * 2747636419u;
    h ^= h >> 16;
    h *= 2246822519u;
    h ^= h >> 13;
    return vec3(
    float((h >>  0) & 0xFF) / 255.0,
    float((h >>  8) & 0xFF) / 255.0,
    float((h >> 16) & 0xFF) / 255.0
    );
}

void main()
{
    outColor = vec4(indexToColor(uint(geometryIndex)), 1.0);
}