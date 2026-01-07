#version 460
#pragma shader_stage(compute)
layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in; // 64
layout (set = 0, binding = 0, rgba32f) uniform highp image3D uTexture;
layout (set = 0, binding = 1) readonly buffer InputBuffer {
    vec3 atomPositions[];
} inputBuffer;


layout (push_constant) uniform Config {
    vec4 bboxMin;
    vec4 bboxMax;
    int numAtoms;
    float radius;
    float scale;
} config;


// https://www.shadertoy.com/view/Ml3Gz8
float smin(float a, float b, float k) {
    float h = a - b;
    h = clamp(0.5 + 0.5*h/k, 0.0, 1.0);
    return mix(a, b, h) - k*h*(1.0-h);
}


void main() {
    ivec3 gID = ivec3(gl_GlobalInvocationID.xyz);

    vec3 size = imageSize(uTexture);
    vec3 uvw = (vec3(gID) + 0.5) / size;
    vec3 worldPos = mix(config.bboxMin.xyz, config.bboxMax.xyz, uvw);
    if (gID.x >= size.x || gID.y >= size.y || gID.z >= size.z) return;

    float value = 1.0f;
    for (int i = 0; i < config.numAtoms; i++) {
        vec3 pos = inputBuffer.atomPositions[i];
        float dist = distance(worldPos, pos);
        value = smin(value, dist - config.radius, config.scale);
    }

    imageStore(uTexture, gID, vec4(vec3(value), 1.0));
    //imageStore(uTexture, gID, vec4(worldPos * config.scale, 1.0));
}