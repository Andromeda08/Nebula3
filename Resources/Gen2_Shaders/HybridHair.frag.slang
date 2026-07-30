#language slang 2026

#include "nbl/Hair.inc.slang"

static const int VizMode_HairColor           = 0;
static const int VizMode_LongScattering      = 1;
static const int VizMode_AzimuthalScattering = 2;
static const int VizMode_CombinedScattering  = 3;
static const int VizMode_Ambient             = 4;
static const int VizMode_Diffuse             = 5;
static const int VizMode_Marschner           = 6;

[[vk::push_constant]]
ConstantBuffer<MeshPushConstants> pc;

float longitudinalM(float thetaH, float shift, float roughness)
{
    var x = thetaH - shift;
    return exp(-(x * x) / (2.0 * roughness * roughness));
}

[shader("fragment")]
float4 main(MeshVertex input) : SV_Target
{
    // Use single light at index 0 for shading, no matter the enable state.
    var light = pc.lights[0];

    // Construct frame
    float3 T = normalize(input.worldTangent);
    var TBN = makeHairFrame(T);

    float3 L = -normalize(light.vector.xyz);
    float3 V = normalize(pc.cameraBuffer.eye.xyz - input.worldPosition);

    // Inclination from normal plane
    float sinThetaI = clamp(dot(L, T), -1.0, 1.0);
    float thetaI    = asin(sinThetaI);

    float sinThetaR = clamp(dot(V, T), -1.0, 1.0);
    float thetaR    = asin(sinThetaR);

    // Longitudinal angles
    float thetaH = (thetaI + thetaR) * 0.5; // -> highlight pos
    float thetaD = (thetaI - thetaR) * 0.5; // -> fresnel & attenuation

    // Per-lobe Gaussians for longitudinal scattering
    var mR   = longitudinalM(thetaH, pc.bsdf.shiftR,   pc.bsdf.roughness);
    var mTT  = longitudinalM(thetaH, pc.bsdf.shiftTT,  pc.bsdf.roughness);
    var mTRT = longitudinalM(thetaH, pc.bsdf.shiftTRT, pc.bsdf.roughness);

    float3 debugColor_Lobes = float3(mR, mTT, mTRT);
    if (pc.vizMode == VizMode_LongScattering)
    {
        return float4(debugColor_Lobes, 1.0);
    }

    // Azimuthal scattering
    float3 L_proj = normalize(L - T * dot(L, T));
    float3 V_proj = normalize(V - T * dot(V, T));

    float cosPhi = dot(L_proj, V_proj);
    float sinPhi = dot(cross(L_proj, V_proj), T);
    float phi    = atan2(sinPhi, cosPhi);

    float  denom0 = (2.0 * pc.bsdf.azimuthalWidth * pc.bsdf.azimuthalWidth);

    float nR   = exp(-(phi * phi)                      / denom0);
    float nTT  = exp(-pow(float.getPi() - abs(phi), 2) / denom0);
    float nTRT = exp(-(phi * phi) / (2.0 * (pc.bsdf.azimuthalWidth * 2.0) * (pc.bsdf.azimuthalWidth * 2.0)));

    float3 debugColor_Azimuthal = float3(nR, nTT, nTRT);
    if (pc.vizMode == VizMode_AzimuthalScattering)
    {
        return float4(debugColor_Azimuthal, 1.0);
    }

    if (pc.vizMode == VizMode_CombinedScattering)
    {
        return float4(debugColor_Lobes + debugColor_Azimuthal, 1.0);
    }

    // Schlick Fresnel approximation
    float3 F0        = float3(pow((1.0 - hairIoR) / (1.0 + hairIoR), 2.0));
    float  cosThetaD = cos(thetaD);
    float3 F         = F0 + (1.0 - F0) * pow(1.0 - cosThetaD, 5.0);

    // Absorption
    float3 absorption = pc.bsdf.absorption.xyz;
    float3 aT  = exp(-absorption);
    float3 aT2 = exp(-absorption * 2.0);

    // Final contribution
    float3 contribR   = (mR   * nR)   * F                           * pc.bsdf.scaleR;
    float3 contribTT  = (mTT  * nTT)  * (1.0 - F) * (1.0 - F) * aT  * pc.bsdf.scaleTT;
    float3 contribTRT = (mTRT * nTRT) * (1.0 - F) * (1.0 - F) * aT2 * pc.bsdf.scaleTRT;

    float3 debugColor_CombinedContrib = contribR + contribTT + contribTRT;

    float3 ambient  = pc.diffuse.rgb * 0.05;
    float3 diffuse  = pc.diffuse.rgb * abs(sinThetaI) * (1.0 / float.getPi()); // was max(0.0, sinThetaI)

    float3 specular = (contribR + contribTRT) * pc.specular.rgb + contribTT * pc.diffuse.rgb;
    // divide by Marschner Jacobian
    // specular /= max(cosThetaD * cosThetaD, 0.01);

    if (pc.vizMode == VizMode_Ambient)
    {
        return float4(ambient, 1.0);
    }
    if (pc.vizMode == VizMode_Diffuse)
    {
        return float4(diffuse, 1.0);
    }
    if (pc.vizMode == VizMode_Marschner)
    {
        return float4(specular, 1.0);
    }

    float3 hairColor = (diffuse + specular) * light.color * light.intensity + ambient;

    return float4(hairColor, 0.85);
}

// [shader("fragment")]
float4 old_main(MeshVertex input) : SV_Target
{
    const float3 lightPos = float3(-75.0, 125.0, 50.0);

    float3 T = normalize(input.worldTangent);
    float3 L = normalize(lightPos - input.worldPosition);
    float3 V = normalize(pc.cameraBuffer.eye.xyz - input.worldPosition);

    float3 diffuse  = pc.useCustomColor == 1 ? pc.diffuse.rgb : input.color;
    float3 specular = pc.useCustomColor == 1 ? pc.specular.rgb : input.color;

    float3 color = kajiyaKay(diffuse, specular, pc.specularFactor, T, L, V);
    return float4(color, 1.0);
}
