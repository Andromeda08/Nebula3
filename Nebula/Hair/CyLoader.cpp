#include "CyLoader.hpp"

#include <bitset>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/fmt/fmt.h>

#include "Core/Random.hpp"

namespace nbl
{
    CyLoader::CyLoader(const std::filesystem::path& path)
    : mPath(path)
    {
        if (!std::filesystem::exists(mPath))
        {
            throw new std::runtime_error(fmt::format("Invalid path: {}", path.string().c_str()));
        }
    }

    HairGeometry CyLoader::load()
    {
        const int result = mHairFile.LoadFromFile(mPath.string().c_str());
        if (result == CY_HAIR_FILE_ERROR_CANT_OPEN_FILE)
        {
            throw std::runtime_error(fmt::format("Cannot open hair file: {}", mPath.string().c_str()));
        }

        HairGeometry geom {};

        // Bit array in file header (https://www.cemyuksel.com/research/hairmodels/)
        const std::bitset<32> bits       = mHairFile.GetHeader().arrays;
        const uint32_t        pointCount = mHairFile.GetHeader().point_count;
        const uint32_t        hairCount  = mHairFile.GetHeader().hair_count;

        // Process vertices
        // ============================
        geom.vertices.reserve(pointCount);
        const float* points = mHairFile.GetPointsArray();
        for (auto i = 0; i < pointCount * 3; i += 3)
        {
            geom.vertices.emplace_back(glm::make_vec3(&points[i]));
        }

        // Process Attributes
        // ============================
        const bool      hasThicknessArray    = bits[2];
        const float*    thicknessArray       = mHairFile.GetThicknessArray();
        const bool      hasTransparencyArray = bits[3];
        const float*    transparencyArray    = mHairFile.GetTransparencyArray();
        const bool      hasColorArray        = bits[4];
        const float*    colorArray           = mHairFile.GetColorsArray();
        const glm::vec3 defaultColor         = glm::make_vec3(mHairFile.GetHeader().d_color);

        geom.attributes.reserve(pointCount);
        for (auto i = 0, colorIdx = 0; i < pointCount; i++, colorIdx += 3)
        {
            geom.attributes.push_back({
                .color        = hasColorArray        ? glm::make_vec3(&colorArray[colorIdx]) : defaultColor,
                .thickness    = hasThicknessArray    ? thicknessArray[i]    : mHairFile.GetHeader().d_thickness,
                .transparency = hasTransparencyArray ? transparencyArray[i] : mHairFile.GetHeader().d_transparency,
            });
        }

        // Process Strands
        // ============================
        const bool hasSegmentsArray = bits[0];
        if (hasSegmentsArray)
        {
            const uint16_t* segmentsArray = mHairFile.GetSegmentsArray();
            for (auto i = 0; i < hairCount; i++)
            {
                geom.strandVertexCounts.push_back(segmentsArray[i]);
            }
        }

        const std::span vertexSpan  = { geom.vertices };
        uint32_t        firstVertex = 0;
        for (auto i = 0; i < hairCount; i++)
        {
            const auto vertexCount = hasSegmentsArray
                ? geom.strandVertexCounts[i] + 1
                : mHairFile.GetHeader().d_segments + 1;

            // Track strand vertex counts if segments array wasn't available
            if (!hasSegmentsArray)
            {
                geom.strandVertexCounts.push_back(vertexCount);
            }

            // HairStrand
            HairStrand strand = {
                .id          = i,
                .vertexCount = vertexCount,
                .vertices    = vertexSpan.subspan(firstVertex, vertexCount),
            };
            geom.strands.push_back(strand);

            for (uint32_t j = 0; j < vertexCount; j++)
            {
                float t = (vertexCount > 1)
                    ? static_cast<float>(j) / static_cast<float>(vertexCount - 1)
                    : 0.0f;
                geom.attributes[firstVertex + j].thickness = glm::mix(0.1f, 0.75f, t);
            }

            // HairStrandlet(s)
            // const uint32_t strandletCount = std::ceil(static_cast<double>(vertexCount) / static_cast<double>(gHairMaxStrandletSize));
            const uint32_t strandletCount = (vertexCount > 1)
                ? ((vertexCount - 1) + (gHairMaxStrandletSize - 1) - 1) / (gHairMaxStrandletSize - 1)
                : 1;
            for (auto j = 0; j < strandletCount; j++)
            {
                // Remainder calc. for the last strandlet.
                // const uint32_t strandletVertexCount = (j != strandletCount - 1)
                //     ? gHairMaxStrandletSize
                //     : (vertexCount - (j * gHairMaxStrandletSize));

                const uint32_t firstStrandletVertex = j * (gHairMaxStrandletSize - 1);  // stride 31
                const uint32_t remaining            = vertexCount - firstStrandletVertex;
                const uint32_t strandletVertexCount = std::min(gHairMaxStrandletSize, remaining);

                HairStrandlet strandlet = {
                    .strandId    = strand.id,
                    .vertexCount = strandletVertexCount,
                    .vertices    = strand.vertices.subspan(firstStrandletVertex, strandletVertexCount),
                };
                geom.strandlets.push_back(strandlet);
            }

            // Hair Strand Description
            HairStrandDesc desc = {
                .strandId       = strand.id,
                .vertexCount    = vertexCount,
                .strandletCount = strandletCount,
                .firstVertex    = firstVertex,
            };
            geom.strandDescs.push_back(desc);

            firstVertex += vertexCount;
        }

        geom.vertexCount    = geom.vertices.size();
        geom.strandCount    = geom.strands.size();
        geom.strandletCount = geom.strandlets.size();
        geom.name           = mPath.filename().stem().string();

        //geom.taskGroupSizeX = static_cast<uint32_t>(std::floor(geom.getStrandCount() / gHairMaxStrandletSize));
        geom.taskGroupSizeX = (geom.strandCount + gHairMaxStrandletSize - 1) / gHairMaxStrandletSize;

        return geom;
    }
}
