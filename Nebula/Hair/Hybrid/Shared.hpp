#pragma once

#include "MarschnerBSDF.hpp"
#include "Hair/HairGeometry.hpp"
#include "Level/Render/Templates.hpp"

namespace nbl
{
    /**
     * Buffers required by the hair renderer
     */
    struct HairRenderer_BDAs
    {
        uint64_t cameraBuffer;
        uint64_t lightsBuffer;
    };

    /**
     * Runtime configurable params
     */
    struct HairRenderingConfig
    {
        // Model index
        uint32_t      hairIndex = 0;

        // Shading
        MarschnerBSDF bsdfParams = {};

        // Task workgroup override
        bool          useCustomWgSize  = false;
        uint32_t      customTaskWgSize = 0;

        // Hybrid rendering config
        bool          isHybridMode           = true;
        float         smallTriangleThreshold = 4.0f;

        // Misc.
        bool          renderHead = true;
        glm::vec3     headColor  = glm::vec3(1.0f);
    };

    /**
     * Resources shared across multiple stages of the Hair Renderer
     */
    struct HairShared
    {
        /**
         * Render resolution used to create resources, also cached as float.
         */
        vk::Extent2D  renderResolution;
        glm::vec2     viewportSize;

        /**
         * The system providing raw hair model data.
         */
        HairModelSystem* hairModels;

        /**
         * Various renderer options.
         */
        HairRenderingConfig config;

        /**
         * Render targets
         */
        static constexpr auto           sVisBufferFormat = vk::Format::eR64Uint;
        static constexpr auto           sColorFormat     = vk::Format::eR32G32B32A32Sfloat;
        static constexpr auto           sDepthFormat     = vk::Format::eD32Sfloat;

        PerFrameArray<SPtr<RHI::Image>> colorTarget;
        PerFrameArray<SPtr<RHI::Image>> depthBuffer;

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
        static constexpr uint32_t          sColorCount = 1024;
        std::array<glm::vec3, sColorCount> colors;
        SPtr<RHI::Buffer>                  colorsBuffer;

        HairShared(const RHI::VulkanRHI* pRHI, HairModelSystem* pHairModelSystem);

    private:
        void init_UtilColors(const RHI::VulkanRHI* pRHI);
    };
}
