#version 460

// Input Attributes
// ========================================
layout (location = 0) in vec4 inWorldPosition;
layout (location = 1) in vec4 inWorldNormal;

// Bound Resources
// ========================================
layout (push_constant) uniform PushConstant {
    vec4 color;
};
layout (set = 0, binding = 0) uniform CameraData {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  position;
    float nearPlane;
    float farPlane;
    float _p0, _p1, _p3;
} cameraData;

// Input Attributes
// ========================================
layout (location = 0) out vec4 outColor;

vec3 gammaCorrection(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

void main()
{
    vec3 V = cameraData.position.xyz - inWorldPosition.xyz;
    vec3 N = normalize(inWorldNormal.xyz);

    vec4 lightPosition = vec4(5.0, -15.0, 15.0, 1.0);
    vec3 lightDirection = -(lightPosition.xyz - inWorldPosition.xyz);

    vec3 L = normalize(lightDirection);
    float lightDistance = length(lightDirection);

    float dotNL = max(dot(N, L), 0.0);

    vec4 color = vec4(color.rgb * dotNL + 0.05 * color.rgb, 1.0);
    vec3 attenuated = (vec3(1.0) * 1500.0f) / (lightDistance * lightDistance);
    outColor = vec4(gammaCorrection(attenuated), 1.0);
}