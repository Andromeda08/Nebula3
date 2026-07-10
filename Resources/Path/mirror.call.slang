#include "lib/bsdf.slang"
#include "lib/frame.slang"

[shader("callable")]
void main(inout BSDF::FnCall bsdf)
{
    if (bsdf.mode == BSDF::MODE_SAMPLE)
    {
        BSDF::Record bRec;
        bRec.params.wi = bsdf.wi;
        bRec.value     = float3(0.0);

        if (Frame::cosTheta(bsdf.wi) <= 0.0)
        {
            bsdf.sampleValue = bRec;
            return;
        }

        bRec.isDiscrete = true;
        bRec.params.wo  = float3(-bsdf.wi.x, -bsdf.wi.y, bsdf.wi.z);
        bRec.value      = float3(1.0);

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
