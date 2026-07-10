#language slang 2026

#include "lib/bsdf.slang"
#include "lib/common.slang"
#include "lib/frame.slang"
#include "lib/sampling.slang"

float getPdf(float3 wi, float3 wo)
{
    if (Frame::cosTheta(wi) <= 0.0 || Frame::cosTheta(wo) <= 0.0)
    {
        return 0.0;
    }
    return getCosineHemispherePdf(wo);
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

        if (Frame::cosTheta(bsdf.wi) <= 0.0)
        {
            bsdf.sampleValue = bRec;
            return;
        }

        bRec.params.wo = toCosineHemisphere(sample);
        bRec.value     = bsdf.albedo;
        bRec.pdf       = getPdf(bsdf.wi, bRec.params.wo);

        bsdf.sampleValue = bRec;
    }
    else if (bsdf.mode == BSDF::MODE_EVAL)
    {
        if (Frame::cosTheta(bsdf.wi) <= 0.0 || Frame::cosTheta(bsdf.wo) <= 0.0)
        {
            bsdf.evalValue = float3(0.0);
            return;
        }
        bsdf.evalValue = bsdf.albedo * (1.0 / float.getPi());
    }
    else
    {
        bsdf.pdfValue = getPdf(bsdf.wi, bsdf.wo);
    }
}
