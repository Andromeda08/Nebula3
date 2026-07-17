#pragma once

#include "TextureManager.hpp"
#include "Render/TonemapPass.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

namespace nbl
{
    class Level;

    struct LevelPathTracerPushConstants
    {
        uint64_t camera;        // Camera BDA
        uint64_t prevCamera;    // Camera BDA
        uint64_t instances;     // Instance BDA
        uint64_t emitters;      // Emitters BDA
        uint64_t emitterPdfs;   // Emitter Discrete PDF BDA
        uint64_t vertices;      // Vertex BDA
        uint64_t indices;       // Index BDA
        uint64_t materials;     // Material.slang BDA
        uint64_t geometryInfos; // Geometry Info BDA
        uint64_t accumulated;   // Current accumulation frame counter
        uint64_t totalFrames;   // Total lifetime frame counter
        uint32_t maxBounces;    // Maximum number of bounces before RR
        uint32_t spp;           // Samples-per-pixel
        int32_t  bDynamicRR;    // Use dynamic rr continuation probability based on throughput?
        float    rrCont;        // RR continuation probability if !bDynamicRR
        uint32_t emitterCount;  // N emitters
        float    jitterX;       // Current frame camera jitter X
        float    jitterY;       // Current frame camera jitter Y
    };

    class LevelPathTracer
    {
    public:
        explicit LevelPathTracer(const SPtr<RHI::VulkanRHI>& rhi, TextureManager* pTextureManager, Level* pLevel);

        void render(const RHI::FrameData& frameData, RHI::CommandList* commandList);

    private:
        static NVSDK_NGX_Resource_VK wrapImage(const SPtr<RHI::Image>& img, const bool readWrite);

        static float halton(uint32_t index, uint32_t base);

        SPtr<RHI::VulkanRHI>            mRHI;

        Level*                          mLevel;

        uint64_t                        mTotalFrames = 0;
        float                           mJitterX = 0.0f;
        float                           mJitterY = 0.0f;

        SPtr<RHI::Descriptor>           mDescriptor;
        UPtr<RHI::RayTracingPipeline2>  mPipeline;

        // 1 SPP Output
        PerFrameArray<SPtr<RHI::Image>> mCurrentOutput;     // RGBA32

        // G-Buffer output
        PerFrameArray<SPtr<RHI::Image>> mNormals;           // RGBA16, World-space
        PerFrameArray<SPtr<RHI::Image>> mDiffuseAlbedo;     // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mSpecularAlbedo;    // RGBA16
        PerFrameArray<SPtr<RHI::Image>> mRoughness;         // R16
        PerFrameArray<SPtr<RHI::Image>> mDepth;             // R32
        PerFrameArray<SPtr<RHI::Image>> mMotionVectors;     // RG16
        PerFrameArray<SPtr<RHI::Image>> mSpecularHitDist;   // R32

        // DLSS RR Output
        PerFrameArray<SPtr<RHI::Image>> mRROutput;          // RGBA16

        UPtr<TonemapPass>               mTonemapPass;
    };
}
