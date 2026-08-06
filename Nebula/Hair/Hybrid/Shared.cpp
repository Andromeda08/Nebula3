#include "Shared.hpp"

#include "GPUScreenSpaceTriangle.hpp"
#include "Core/Random.hpp"

namespace nbl
{
    HairShared::HairShared(const RHI::VulkanRHI* pRHI, HairModelSystem* pHairModelSystem)
    : hairModels(pHairModelSystem)
    {
        renderResolution = pRHI->getSwapchain()->getProperties().extent;
        viewportSize = { static_cast<float>(renderResolution.width), static_cast<float>(renderResolution.height) };

        const auto& hairGeom = pHairModelSystem->getHairGeometry(config.hairIndex);

        /**
             * Worse case (vertexCount * 2) amount of quads are generated, however realistically we will
             * never reach this, even when dynamic LoD is implemented.
             * If it ever reaches 50% occupancy it's cooked anyway...
             */
        const uint32_t trianglesBufferSize = (hairGeom.getVertexCount() * 2) * sizeof(GPUScreenSpaceTriangle);

        for (size_t i = 0; i < RHI::gFramesInFlight; i++)
        {
            // Render targets
            colorTarget[i] = makeRenderTarget(pRHI, fmt::format("Hair_ColorTarget_{}", i), sColorFormat);
            depthBuffer[i] = makeRenderTarget(pRHI, fmt::format("Hair_DepthBuffer_{}", i), sDepthFormat);

            // Auxiliary buffers
            currentBufferSize[i] = trianglesBufferSize;
            trianglesBuffer[i]   = pRHI->createBuffer({
                .size = currentBufferSize[i],
                .type = RHI::BufferType::Storage,
                .label = fmt::format("Hair_TrianglesBuffer_{}", i),
            });

            smallTriangleCounterBuffer[i] = pRHI->createBuffer({
                .size = sizeof(uint32_t),
                .type = RHI::BufferType::Storage,
                .label = fmt::format("Hair_SmallTriangleBuffer_{}", i),
            });
        }

        init_UtilColors(pRHI);
    }

    void HairShared::init_UtilColors(const RHI::VulkanRHI* pRHI)
    {
        // Generate colors
        for (size_t i = 0; i < sColorCount; i++)
        {
            const auto color = Random::getColor();
            colors[i]        = glm::xyz(color);
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
