#pragma once

// CyLoader.hpp
// This file contains the code for loading cyHairFiles.
// https://www.cemyuksel.com/research/hairmodels/
// ============================================================

#include <cyHairFile.h>
#include <filesystem>
#include "HairGeometry.hpp"

namespace nbl
{
    class CyLoader
    {
    public:
        explicit CyLoader(const std::filesystem::path& path);

        [[nodiscard]] HairGeometry load();

    private:
        std::filesystem::path   mPath;
        cyHairFile              mHairFile;
    };
}
