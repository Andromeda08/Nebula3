#include "lib/common.slang"
#include "lib/bsdf.slang"
#include "lib/frame.slang"

struct FresnelResult
{
    float cosThetaT;
    float F;
};

FresnelResult fresnel(const float cosThetaI, const float etaI, const float etaT)
{
    FresnelResult result;
    result.cosThetaT = 0.0;
    result.F = 0.0;

    // Snell's Law
    const float sinThetaI = sqrt(max(0.0, 1.0 - cosThetaI * cosThetaI));
    const float sinThetaT = (/* rel. IoR/ */ etaI / etaT) * sinThetaI;

    float cosThetaT = sqrt(max(0.0, 1.0 - sinThetaT * sinThetaT));
    result.cosThetaT = cosThetaT;

    // Total internal reflection case
    if (sinThetaT >= 1.0)
    {
        result.F = 1.0;
        return result;
    }

    // Fresnel reflectance formula for dielectrics
    const float rParallel      = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
    const float rPerpendicular = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));
    result.F = 0.5 * (rParallel * rParallel + rPerpendicular * rPerpendicular);

   return result;
}

[shader("callable")]
void main(inout BSDF::FnCall bsdf)
{
    if (bsdf.mode == BSDF::MODE_SAMPLE)
    {
        float2 sample = rnd2(bsdf.seed);

        BSDF::Record bRec;
        bRec.params.wi = bsdf.wi;
        bRec.value     = float3(0.0);

        float  cosThetaI = Frame::cosTheta(bsdf.wi);
        float3 normal    = float3(0.0, 0.0, 1.0);

        float etaI = bsdf.material.dielectric_ExtIoR;
        float etaT = bsdf.material.dielectric_IntIoR;

        // We have to decide if we're exiting or entering the surface.
        if (/* Swap etaI and etaT when exiting */ cosThetaI < 0.0)
        {
            float temp = etaI;
            etaI = etaT;
            etaT = temp;

            cosThetaI  = abs(cosThetaI);
            normal.z = -1.0;
        }

        const FresnelResult f = fresnel(cosThetaI, etaI, etaT);
        const float cosThetaT = f.cosThetaT;
        const float F         = f.F;

        // Reflection
        if (sample.x < F)
        {
            bRec.params.wo = float3(-bsdf.wi.x, -bsdf.wi.y, bsdf.wi.z);
            bRec.value     = float3(1.0);
        }
        // Refraction
        else
        {
            const float eta = etaI / etaT;
            bRec.params.wo = (-eta * bsdf.wi) + ((eta * cosThetaI - cosThetaT) * normal);
            bRec.value     = float3(eta * eta);
        }

        bRec.isDiscrete = true;

        bsdf.sampleValue = bRec;
    }
    else if (bsdf.mode == BSDF::MODE_EVAL)
    {
        bsdf.evalValue = float3(0.0);
    }
    else
    {
        bsdf.pdfValue = 0.0;
    }
}
