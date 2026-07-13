#pragma once

#include "types.slang"

namespace BSDF
{
    // Function Params:
    // float3 wi
    static const uint MODE_SAMPLE = 0u;

    // Function Params:
    // - float3 wi
    // - float3 wo
    static const uint MODE_EVAL   = 1u;

    // Function Params:
    // - float3 wi
    // - float3 wo
    static const uint MODE_PDF    = 2u;

    struct Params
    {
        float3 wi;
        float3 wo;
    };

    struct Record
    {
        Params  params;
        bool    isDiscrete;
        float3  value;

        float   pdf;

        uint    seed;
    };

    struct FnCall
    {
        float3          albedo;
        GPUMaterialData material;
        uint            mode;
        uint            seed;

        float3          wi;
        float3          wo;

        Record          sampleValue;
        float3          evalValue;
        float           pdfValue;
    };

    Record sample(GPUMaterialData material, uint seed, float3 wi, float3 albedo)
    {
        FnCall call;
        call.albedo = albedo; // material.solidColor.rgb;
        call.mode   = MODE_SAMPLE;
        call.seed   = seed;
        call.wi     = wi;

        CallShader(material.bsdfIndex, call);

        call.sampleValue.seed = call.seed;
        return call.sampleValue;
    }

    float3 eval(GPUMaterialData material, uint seed, Params params, float3 albedo)
    {
        FnCall call;
        call.albedo = albedo; // material.solidColor.rgb;
        call.mode   = MODE_EVAL;
        call.seed   = seed;
        call.wi     = params.wi;
        call.wo     = params.wo;

        CallShader(material.bsdfIndex, call);

        return call.evalValue;
    }

    float pdf(GPUMaterialData material, uint seed, Params params)
    {
        FnCall call;
        call.mode = MODE_PDF;
        call.seed = seed;
        call.wi   = params.wi;
        call.wo   = params.wo;

        CallShader(material.bsdfIndex, call);

        return call.pdfValue;
    }
}
