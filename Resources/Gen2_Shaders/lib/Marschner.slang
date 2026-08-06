#pragma once

#include "Camera.slang"
#include "Emitter.slang"

static const float hairIoR = 1.55;

namespace MarschnerDebug
{
    static const int HairColor           = 0;
    static const int LongScattering      = 1;
    static const int AzimuthalScattering = 2;
    static const int CombinedScattering  = 3;
    static const int Ambient             = 4;
    static const int Diffuse             = 5;
    static const int Marschner           = 6;
}

struct MarschnerParams
{
    float3 diffuseTint;
    float3 specularTint;

    float3 absorption;

    float  roughness;
    float  azimuthalWidth;
    float  shiftR;
    float  shiftTT;
    float  shiftTRT;
    float  scaleR;
    float  scaleTT;
    float  scaleTRT;
};

namespace detail
{
    float longitudinalM(float thetaH, float shift, float roughness)
    {
        var x = thetaH - shift;
        return exp(-(x * x) / (2.0 * roughness * roughness));
    }
}

float3 MarschnerBSDF(
    GPULightData light, GPUCameraData camera,
    float3 wPosition, float3 wTangent,
    MarschnerParams bsdf
)
{
    float3 T = normalize(wTangent);
    float3 L = -normalize(light.vector.xyz);
    float3 V = normalize(camera.eye.xyz - wPosition);

    // Inclination from normal plane
    float sinThetaI = clamp(dot(L, T), -1.0, 1.0);
    float thetaI    = asin(sinThetaI);

    float sinThetaR = clamp(dot(V, T), -1.0, 1.0);
    float thetaR    = asin(sinThetaR);

    // Longitudinal angles
    float thetaH = (thetaI + thetaR) * 0.5; // -> highlight pos
    float thetaD = (thetaI - thetaR) * 0.5; // -> fresnel & attenuation

    // Per-lobe Gaussians for longitudinal scattering
    var mR   = detail::longitudinalM(thetaH, bsdf.shiftR,   bsdf.roughness);
    var mTT  = detail::longitudinalM(thetaH, bsdf.shiftTT,  bsdf.roughness);
    var mTRT = detail::longitudinalM(thetaH, bsdf.shiftTRT, bsdf.roughness);

    // Azimuthal scattering
    float3 L_proj = normalize(L - T * dot(L, T));
    float3 V_proj = normalize(V - T * dot(V, T));

    float cosPhi = dot(L_proj, V_proj);
    float sinPhi = dot(cross(L_proj, V_proj), T);
    float phi    = atan2(sinPhi, cosPhi);

    float denom0 = (2.0 * bsdf.azimuthalWidth * bsdf.azimuthalWidth);

    float nR   = exp(-(phi * phi)                      / denom0);
    float nTT  = exp(-pow(float.getPi() - abs(phi), 2) / denom0);
    float nTRT = exp(-(phi * phi) / (2.0 * (bsdf.azimuthalWidth * 2.0) * (bsdf.azimuthalWidth * 2.0)));

    float3 debugColor_Azimuthal = float3(nR, nTT, nTRT);

    // Schlick Fresnel approximation
    float3 F0        = float3(pow((1.0 - hairIoR) / (1.0 + hairIoR), 2.0));
    float  cosThetaD = cos(thetaD);
    float3 F         = F0 + (1.0 - F0) * pow(1.0 - cosThetaD, 5.0);

    // Absorption
    float3 absorption = bsdf.absorption.xyz;
    float3 aT  = exp(-absorption);
    float3 aT2 = exp(-absorption * 2.0);

    // Final contribution
    float3 contribR   = (mR   * nR)   * F                           * bsdf.scaleR;
    float3 contribTT  = (mTT  * nTT)  * (1.0 - F) * (1.0 - F) * aT  * bsdf.scaleTT;
    float3 contribTRT = (mTRT * nTRT) * (1.0 - F) * (1.0 - F) * aT2 * bsdf.scaleTRT;

    float3 debugColor_CombinedContrib = contribR + contribTT + contribTRT;

    float3 ambient  = bsdf.diffuseTint * 0.05;
    float3 diffuse  = bsdf.diffuseTint * abs(sinThetaI) * (1.0 / float.getPi()); // was max(0.0, sinThetaI)

    float3 specular = (contribR + contribTRT) * bsdf.specularTint + contribTT * bsdf.diffuseTint;
    // divide by Marschner Jacobian
    // specular /= max(cosThetaD * cosThetaD, 0.01);

    return (diffuse + specular) * light.color * light.intensity + ambient;
}
