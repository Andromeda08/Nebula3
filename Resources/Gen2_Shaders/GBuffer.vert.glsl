#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_GOOGLE_include_directive : require

#define nbl_INSTANCE_INDIRECTION
#include "nbl/Instance.inc.glsl"

#define nbl_CAMERA_HISTORY
#include "nbl/Camera.inc.glsl"

#include "nbl/Material.inc.glsl"

// Input Attributes
// ======================================
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

// Output Attributes
// ======================================
layout (location = 0) out uint outInstanceIndex;
layout (location = 1) out vec3 outWorldPosition;
layout (location = 2) out vec3 outWorldPositionPrev;
layout (location = 3) out vec3 outWorldNormal;
layout (location = 4) out vec4 outWorldTangent;
layout (location = 5) out vec2 outUV;
layout (location = 6) out vec4 outCurrClip;
layout (location = 7) out vec4 outPrevClip;

layout (push_constant) uniform PushConstants
{
    InstanceBuffer            instances;
    InstanceIndirectionBuffer instanceIndirectionMap;
    CameraBuffer              camera;
    HistoryCameraBuffer       previousCamera;
    MaterialBuffer            materials;
    uvec2                     renderRes;
};

void main()
{
    // Resolve instance buffer index
    uint instanceIndex = instanceIndirectionMap.data[gl_InstanceIndex];
    outInstanceIndex   = instanceIndex;

    GPUInstanceData instanceData = instances.data[instanceIndex];

    // Current and Previous (World) Position
    vec4 position = vec4(inPosition, 1.0);

    vec4 worldPosition = (instanceData.model * position);
    outWorldPosition = worldPosition.xyz;

    vec4 worldPositionPrev = (instanceData.modelPrevious * position);
    outWorldPositionPrev = worldPositionPrev.xyz;

    // Current and Previous (Clip) Position
    vec4 currClip = camera.data.proj * camera.data.view * worldPosition;
    outCurrClip = currClip;

    vec4 prevClip = previousCamera.data.proj * previousCamera.data.view * worldPositionPrev;
    outPrevClip = prevClip;
    
    // TBN
    float modelHandedness = sign(determinant(mat3(instanceData.model)));
    float bitangentSign = inTangent.w * modelHandedness;

    mat3 normalMatrix = transpose(mat3(instanceData.modelInverse));
    outWorldNormal    = normalize(normalMatrix * inNormal);
    outWorldTangent   = vec4(normalize(normalMatrix * inTangent.xyz), bitangentSign);

    // UV
    outUV = inUV;

    gl_Position = currClip;
}
