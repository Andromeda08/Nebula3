#include "Geometry.hpp"

#include <numbers>

#include "TangentGeneration.hpp"

namespace nbl
{
    #pragma region "Cube index and vertex data"

    static const std::vector<uint32_t> gCubeIndices = {
        0,  1,  2,  2,  3,  0,
        4,  5,  6,  6,  7,  4,
        8,  9, 10, 10, 11,  8,
       12, 13, 14, 14, 15, 12,
       16, 17, 18, 18, 19, 16,
       20, 21, 22, 22, 23, 20,
   };

    static const std::vector<Vertex> gCubeVertices = {
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    };

    #pragma endregion

    // Geometry Class
    // ============================================================

    Geometry::Geometry(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::string name)
    : mVertices(std::move(vertices))
    , mIndices(std::move(indices))
    , mName(std::move(name))
    {
        mVertexCount   = static_cast<uint32_t>(mVertices.size());
        mIndexCount    = static_cast<uint32_t>(mIndices.size());
        mTriangleCount = mIndexCount / 3;

        computeBoundingBox();
    }

    const BoundingBox& Geometry::getBoundingBox() const noexcept
    {
        return mBoundingBox;
    }

    const std::vector<Vertex>& Geometry::getVertices() const noexcept
    {
        return mVertices;
    }

    uint32_t Geometry::getVertexCount() const noexcept
    {
        return mVertexCount;
    }

    const std::vector<uint32_t>& Geometry::getIndices() const noexcept
    {
        return mIndices;
    }

    uint32_t Geometry::getIndexCount() const noexcept
    {
        return mIndexCount;
    }

    uint32_t Geometry::getTriangleCount() const noexcept
    {
        return mTriangleCount;
    }

    const std::string& Geometry::getName() const noexcept
    {
        return mName;
    }

    void Geometry::generateTangents()
    {
        // Do somthing about this :(
        std::vector<glm::vec3> p = mVertices | std::views::transform(&Vertex::position) | std::ranges::to<std::vector>();
        std::vector<glm::vec3> n = mVertices | std::views::transform(&Vertex::normal) | std::ranges::to<std::vector>();
        std::vector<glm::vec2> u = mVertices | std::views::transform(&Vertex::uv) | std::ranges::to<std::vector>();
        std::vector<glm::vec4> t;

        Tangent::generateTangents(mVertexCount, {
            .positions = &p,
            .normals   = &n,
            .texcoords = &u,
            .indices   = &mIndices,
            .tangents  = &t,
        });

        for (auto i = 0; i < mVertexCount; i++)
        {
            mVertices[i].tangent = t[i];
        }
    }

    void Geometry::computeBoundingBox()
    {
        mBoundingBox.reset();
        for (const auto& vertex : mVertices)
        {
            mBoundingBox.expandBy(glm::vec3(vertex.position));
        }
    }

    // Cube Geometry
    // ============================================================
    #pragma region "Cube"

    SPtr<Geometry> Cube::createGeometry(std::string name, const float sideLength)
    {
        auto geometry = makeShared<Geometry>(generateVertices(sideLength), generateIndices(), name);
        geometry->generateTangents();
        return geometry;
    }

    std::vector<Vertex> Cube::generateVertices(const float sideLength)
    {
        std::vector<Vertex> vertices = gCubeVertices;

        const float scale = sideLength / 2.0f;
        for (auto& vertex : vertices)
        {
            vertex.position *= scale;
        }

        return vertices;
    }

    std::vector<uint32_t> Cube::generateIndices()
    {
        return gCubeIndices;
    }

    #pragma endregion

    // Sphere Geometry
    // ============================================================
    #pragma region "Sphere"

    SPtr<Geometry> Sphere::createGeometry(std::string name, const float radius, const int32_t resolution)
    {
        auto geometry = makeShared<Geometry>(generateVertices(resolution, resolution, radius), generateIndices(resolution, resolution), name);
        geometry->generateTangents();
        return geometry;
    }

    std::vector<Vertex> Sphere::generateVertices(const int32_t stackCount, const int32_t sectorCount, const float radius)
    {
        std::vector<glm::vec3> vertices, normals;
        std::vector<glm::vec2> uvs;

        const float sectorStep = 2.0f * std::numbers::pi_v<float> / static_cast<float>(sectorCount);
        const float stackStep  = std::numbers::pi_v<float> / static_cast<float>(stackCount);

        const float radiusOverOne = 1.0f / radius;

        for (int32_t i = 0; i <= stackCount; i++)
        {
            float x, y, z;
            const float stackAngle = std::numbers::pi_v<float> / 2 - static_cast<float>(i) * stackStep;
            const float xy         = radius * cosf(stackAngle);
            z                      = radius * sinf(stackAngle);

            for (int32_t j = 0; j <= sectorCount; j++)
            {
                const float sectorAngle = static_cast<float>(j) * sectorStep;

                x = xy * cosf(sectorAngle);
                y = xy * sinf(sectorAngle);

                vertices.emplace_back(x, y, z);
                normals.emplace_back(x * radiusOverOne, y * radiusOverOne, z * radiusOverOne);
                uvs.emplace_back(
                    static_cast<float>(j) / static_cast<float>(sectorCount),
                    static_cast<float>(i) / static_cast<float>(stackCount));
            }
        }

        std::vector<Vertex> result;
        for (uint32_t i = 0; i < vertices.size(); i++)
        {
            result.push_back({ vertices[i], normals[i], uvs[i] });
        }

        return result;
    }

    std::vector<uint32_t> Sphere::generateIndices(const int32_t stackCount, const int32_t sectorCount)
    {
        std::vector<uint32_t> indices;

        for (int32_t i = 0; i < stackCount; i++)
        {
            uint32_t k1 = i * (sectorCount + 1);
            uint32_t k2 = k1 + sectorCount + 1;

            for (int32_t j = 0; j < sectorCount; j++, k1++, k2++)
            {
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }
                if (i != (stackCount - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }
        return indices;
    }

    #pragma endregion
}
