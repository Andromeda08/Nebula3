#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#define FXAA_GREEN_AS_LUMA 1

#include "lib/FXAA.inc.hlsl"

static const float fxaaSubpix           = 0.75;
static const float fxaaEdgeThreshold    = 0.166;
static const float fxaaEdgeThresholdMin = 0.0833;

struct FXAAPushConstant
{
    float rcpX;
    float rcpY;
};

[[vk::push_constant]] FXAAPushConstant pc;

[[vk::binding(0)]] Sampler2D inputImage;

[shader("fragment")]
float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_Target
{
    const float2 rcp = float2(pc.rcpX, pc.rcpY);
    const float2 uv  = inUV;
    FxaaTex tex = {
        inputImage.__getSampler(),
        inputImage.__getTexture()
    };
    return FxaaPixelShader(uv, 0, tex, tex, tex, rcp, 0, 0, 0, fxaaSubpix, fxaaEdgeThreshold, fxaaEdgeThresholdMin, 0, 0, 0, 0);
}
