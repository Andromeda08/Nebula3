#include "Geometry.hpp"

#include <meshoptimizer.h>
#include <glm/gtc/type_ptr.hpp>

namespace nbl
{
    Geometry::Geometry(const GeometryCreateInfo& createInfo)
    : mName(createInfo.name)
    , mVertices(createInfo.vertices)
    , mAttributes(createInfo.attributes)
    , mIndices(createInfo.indices)
    {
        optimizeGeometry();

        computeBoundingBox();
        computeShadowIndices();
        generateMeshlets();
    }

    void Geometry::optimizeGeometry()
    {
        const auto indexCount  = getIndexCount();
        const auto vertexCount = getVertexCount();

        meshopt_optimizeVertexCache(mIndices.data(), mIndices.data(), indexCount, vertexCount);
        meshopt_optimizeOverdraw(
            mIndices.data(),
            mIndices.data(),
            indexCount,
            &mVertices[0].position.x,
            vertexCount,
            sizeof(Vertex),
            1.05f
        );
    }

    void Geometry::computeBoundingBox()
    {
        mBoundingBox.reset();
        for (const auto& [ position ] : mVertices)
        {
            mBoundingBox.expandBy(position);
        }
    }

    void Geometry::computeShadowIndices()
    {
        const auto indexCount  = getIndexCount();
        const auto vertexCount = getVertexCount();

        mShadowIndices.resize(indexCount);

        meshopt_generateShadowIndexBuffer(
            mShadowIndices.data(),
            mIndices.data(),
            indexCount,
            &mVertices[0].position.x,
            vertexCount,
            sizeof(Vertex),
            sizeof(Vertex)
        );
    }

    void Geometry::generateMeshlets()
    {
        const auto indexCount  = getIndexCount();
        const auto vertexCount = getVertexCount();

        const size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, sMeshletMaxVertices, sMeshletMaxTriangles);

        std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
        std::vector<uint32_t>        meshletVertices(indexCount);
        std::vector<uint8_t>         meshletTriangles(indexCount);

        const size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            mIndices.data(),
            indexCount,
            &mVertices[0].position.x,
            vertexCount,
            sizeof(Vertex),
            sMeshletMaxVertices,
            sMeshletMaxTriangles,
            sMeshletConeWeight
        );

        const meshopt_Meshlet& last = meshlets[meshletCount - 1];
        meshletVertices.resize(last.vertex_offset + last.vertex_count);
        meshletTriangles.resize(last.triangle_offset + last.triangle_count * 3);
        meshlets.resize(meshletCount);

        mMeshlets.reserve(meshletCount);
        for (auto i = 0; i < meshletCount; i++)
        {
            const auto& meshlet = meshlets[i];
            meshopt_optimizeMeshlet(
                &meshletVertices[meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count,
                meshlet.vertex_count
            );

            const meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                &meshletVertices[meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count,
                &mVertices[0].position.x,
                vertexCount,
                sizeof(Vertex)
            );

            mMeshlets.push_back({
                .firstVertex    = meshlet.vertex_offset,
                .vertexCount    = meshlet.vertex_count,
                .firstTriangle  = meshlet.triangle_offset,
                .triangleCount  = meshlet.triangle_count,
                .center         = glm::make_vec3(bounds.center),
                .radius         = bounds.radius,
                .coneAxis       = glm::make_vec3(bounds.cone_axis),
                .coneCutoff     = bounds.cone_cutoff
            });
        }

        mMeshletVertices  = std::move(meshletVertices);
        mMeshletTriangles = std::move(meshletTriangles);
    }
}
