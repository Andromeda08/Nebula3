#pragma once

struct MoleculeRenderingOptions
{
    bool renderStructure      = true;
    bool renderSurface        = true;
    bool shouldRecalculateSDF = false;
    bool hasCalculatedSDF     = false;
    bool useSubsurfaceScattering = true;
};
