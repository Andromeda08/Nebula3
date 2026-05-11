#pragma once

#include <glm/glm.hpp>

#include "Core/Random.hpp"
#include "Core/Types.hpp"
#include "Hair/HairGeometry.hpp"
#include "VulkanRHI/Buffer.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class HairModelSystem;

    enum class RenderingMode : uint8_t
    {
        DefaultHybrid   = 0,
        DebugQuads      = 1,
        DebugStrands    = 2,
        DebugStrandlets = 3,
    };

    /**
     * A single screen-space triangle.
     * The mesh shader outputs small triangles in this format for the software path to consume.
     * A vertex of a triangle is stored as such: [x pixel, y pixel, depth]
     */
    struct ScreenSpaceTriangle
    {
        glm::vec3 v0;
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 tangent;
        glm::vec3 color;
        uint32_t  primitiveId;
    };

    struct HairRenderingConfig
    {
        RenderingMode renderingMode    = RenderingMode::DefaultHybrid;
        vk::Extent2D  renderResolution = { 1920, 1080 };

        // Hair Model Index
        int32_t hairIndex = 0;

        // Color override options
        bool      overrideColors = false;
        glm::vec3 diffuse        = glm::vec3(0.32549f, 0.23921f, 0.20784f);
        glm::vec3 specular       = glm::vec3(0.41568f, 0.30588f, 0.21960f);
    };

    /**
     * Resources shared across multiple stages of the Hair Renderer
     */
    struct HairShared
    {
        /**
         * The system providing raw hair model data.
         */
        HairModelSystem* hairModels;

        /**
         * Various renderer options.
         */
        HairRenderingConfig config;

        /**
         * Visibility buffers written to by the mesh and compute pipeline.
         */
        static constexpr auto           sVisBufferFormat = vk::Format::eR64Uint;
        PerFrameArray<SPtr<RHI::Image>> visibilityBuffer;

        /**
         * Storage buffer containing an array of ScreenSpaceTriangles.
         * Output by the mesh shader and consumed by the software rasterizer path.
         */
        PerFrameArray<uint32_t>          currentBufferSize;
        PerFrameArray<SPtr<RHI::Buffer>> trianglesBuffer;
        PerFrameArray<SPtr<RHI::Buffer>> smallTriangleCounterBuffer;

        /**
         * An array of colors that can be used for a variety of debug or visualization purposes.
         * e.g. index, primitive ID, strand...
         */
        static constexpr size_t            sColorCount = 1024;
        std::array<glm::vec3, sColorCount> colors;
        SPtr<RHI::Buffer>                  colorsBuffer;

        HairShared(const RHI::VulkanRHI* pRHI, HairModelSystem* pHairModelSystem)
        : hairModels(pHairModelSystem)
        {
            config.renderResolution = pRHI->getSwapchain()->getProperties().extent;

            const auto& hairGeom = pHairModelSystem->getHairGeometry(config.hairIndex);

            /**
             * Worse case (vertexCount * 2) amount of quads are generated, however realistically we will
             * never reach this, even when dynamic LoD is implemented.
             * If it ever reaches 50% occupancy it's cooked anyway...
             */
            const uint32_t trianglesBufferSize = (hairGeom.getVertexCount() * 2) * sizeof(ScreenSpaceTriangle);

            for (size_t i = 0; i < RHI::gFramesInFlight; i++)
            {
                using I = vk::ImageUsageFlagBits;
                visibilityBuffer[i] = pRHI->createImage({
                    .extent     = config.renderResolution,
                    .format     = sVisBufferFormat,
                    .usageFlags = I::eStorage | I::eTransferDst,
                    .debugName  = fmt::format("Hair_VisibilityBuffer_{}", i),
                });

                currentBufferSize[i] = trianglesBufferSize;
                trianglesBuffer[i] = pRHI->createBuffer({
                    .size  = currentBufferSize[i],
                    .type  = RHI::BufferType::Storage,
                    .label = fmt::format("Hair_TrianglesBuffer_{}", i),
                });

                smallTriangleCounterBuffer[i] = pRHI->createBuffer({
                    .size  = sizeof(uint32_t),
                    .type  = RHI::BufferType::Storage,
                    .label = "Hair_SmallTriangleBuffer",
                });
            }

            // Create util colors
            // ============================
            {
                // Generate colors
                for (size_t i = 0; i < sColorCount; i++)
                {
                    const auto color = Random::getColor();
                    colors[i] = glm::xyz(color);
                }

                // Upload to GPU
                const uint64_t bufferSize = colors.size() * sizeof(glm::vec3);
                colorsBuffer = pRHI->createBuffer({
                    .size  = bufferSize,
                    .type  = RHI::BufferType::Storage,
                    .label = "Hair_UtilColorsBuffer",
                });
                pRHI->immediate_uploadToBuffer(colorsBuffer.get(), colors.data(), bufferSize);
            }
        }
    };
}
