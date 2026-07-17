#pragma once

#include "Camera.slang"
#include "Constants.slang"
#include "Emitter.slang"
#include "Geometry.slang"
#include "InstanceData.slang"
#include "Material.slang"

namespace nbl
{
    float4 computeMotionVector(float4 currClip, float4 prevClip, float deltaViewZ, float2 renderRes)
    {
        float2 currNDC = currClip.xy / currClip.w;
        float2 currUV  = currNDC * 0.5 + 0.5;

        float2 prevNDC = prevClip.xy / prevClip.w;
        float2 prevUV  = prevNDC * 0.5 + 0.5;

        return float4((prevUV - currUV) * renderRes, deltaViewZ, 0);
    }
}
