#include "RHIFeatures.hpp"

#include "Detail/DeviceExtensions.hpp"

namespace RHI
{
    RHIFeatures gFeatures = {};

    void RHIFeatures::updateFeatureSetsByExtensions(const DeviceExtensions& exts)
    {
        // Ray Tracing
        rayTracing = exts.isActive<AccelerationStructureKHR>()
            && exts.isActive<RayTracingPipeline>()
            && exts.isActive<RayQuery>()
            && exts.isActive<RayTracingMaintenance1>()
            && exts.isActive<RayTracingPositionFetch>();

        // NV Ray Tracing
        nvRayTracing = exts.isActive<RayTracingLinearSweptSpheresNV>();

        // Geometry, Tessellation Shaders
        geomTess = exts.getFeatures().geometryShader
            && exts.getFeatures().tessellationShader;

        // Mesh Shaders
        meshShaders = exts.isActive<MeshShaderEXT>();

        const auto counts = static_cast<vk::SampleCountFlags::MaskType>(exts.getProperties().limits.sampledImageColorSampleCounts);
        maxMSAA = static_cast<vk::SampleCountFlagBits>(std::bit_floor(counts));
    }
}
