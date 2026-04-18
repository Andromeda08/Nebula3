#include "Geometry.hpp"

#include <numbers>

#include "glm/gtc/constants.hpp"

#pragma region "Cube index and vertex data"

std::vector<uint32_t> Cube::sCubeIndices = {
     0,  1,  2,  2,  3,  0,
     4,  5,  6,  6,  7,  4,
     8,  9, 10, 10, 11,  8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
};

std::vector<Vertex> Cube::sCubeVertices = {
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

Geometry::Geometry(const GeometryCreateInfo& createInfo)
: mVertices(createInfo.vertices)
, mIndices(createInfo.indices)
, mName(createInfo.name)
{
    mVertexCount = static_cast<uint32_t>(mVertices.size());
    mIndexCount  = static_cast<uint32_t>(mIndices.size());

    computeBoundingBox();
}

void Geometry::computeBoundingBox()
{
    mBoundingBox.reset();
    for (const auto& vertex : mVertices)
    {
        mBoundingBox.expandBy(vertex.position);
    }
}

Cube::Cube(const Params& params)
: Geometry()
{
    mVertices    = generateVertices(params.scale);
    mVertexCount = static_cast<uint32_t>(mVertices.size());

    mIndices     = sCubeIndices;
    mIndexCount  = static_cast<uint32_t>(sCubeIndices.size());

    mName = sName;
}

std::vector<Vertex> Cube::generateVertices(const float scale)
{
    std::vector<Vertex> vertices = Cube::sCubeVertices;
    for (auto& vertex : vertices)
    {
        vertex.position *= scale;
    }
    return vertices;
}

Sphere::Sphere(const Params& params)
: Geometry()
{
    mVertices    = generateVertices(params.tesselationX, params.tesselationY, params.radius);
    mVertexCount = static_cast<uint32_t>(mVertices.size());

    mIndices     = generateIndices(params.tesselationX, params.tesselationY);
    mIndexCount  = static_cast<uint32_t>(mIndices.size());

    mName = sName;
}

std::vector<Vertex> Sphere::generateVertices(const int32_t stackCount, const int32_t sectorCount, const float radius)
{
    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> uvs;

    float sector_step = 2.0f * std::numbers::pi_v<float> / (float) sectorCount;
    float stack_step  = std::numbers::pi_v<float> / (float) stackCount;
    float sector_angle, stack_angle;

    float r_inv = 1.0f / radius;

    for (int32_t i = 0; i <= stackCount; i++)
    {
        float x, y, z, xy;
        stack_angle = std::numbers::pi_v<float> / 2 - (float) i * stack_step;
        xy = radius * cosf(stack_angle);
        z = radius * sinf(stack_angle);

        for (int32_t j = 0; j <= sectorCount; j++)
        {
            sector_angle = (float) j * sector_step;

            x = xy * cosf(sector_angle);
            y = xy * sinf(sector_angle);

            vertices.emplace_back(x, y, z);
            normals.emplace_back(x * r_inv, y * r_inv, z * r_inv);
            uvs.emplace_back((float) j / (float) sectorCount, (float) i / (float) stackCount);
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
    uint32_t k1, k2;

    for (int32_t i = 0; i < stackCount; i++)
    {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

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

Cylinder::Cylinder(const Params& params)
: Geometry()
{
    // TODO: UVs are not generated yet
    mVertices    = generateVertices(params.tesselation, params.radius, params.height);
    mVertexCount = static_cast<uint32_t>(mVertices.size());

    mIndices     = generateIndices(params.tesselation);
    mIndexCount  = static_cast<uint32_t>(mIndices.size());

    mName = sName;
}

std::vector<Vertex> Cylinder::generateVertices(const uint32_t segments, const float radius, const float height) noexcept
{
    std::vector<Vertex> vertices;

    vertices.push_back({
        .position = { 0.0f, -height / 2.0f, 0.0f },
        .normal   = { 0.0f, -1.0f, 0.0f },
        .uv       = {}
    });

    vertices.push_back({
        .position = { 0.0f, height / 2.0f, 0.0f },
        .normal   = { 0.0f, 1.0f, 0.0f },
        .uv       = {}
    });

    // Circle
    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (auto i = 0; i < segments; i++)
    {
        glm::vec3 position = {
            glm::cos(i * step) * radius,
            -height / 2.0f,
            glm::sin(i * step) * radius
        };

        // Bottom ring
        vertices.push_back({
            .position = position,
            .normal   = { 0.0f, -1.0f, 0.0f },
            .uv       = {},
        });
        vertices.push_back({
            .position = position,
            .normal   = glm::normalize(position - glm::vec3(0, -height / 2.0f, 0)),
            .uv       = {},
        });

        // Top ring
        position.y = height / 2.0f;
        vertices.push_back({
            .position = position,
            .normal   = { 0.0f, 1.0f, 0.0f },
            .uv       = {},
        });
        vertices.push_back({
            .position = position,
            .normal   = glm::normalize(position - glm::vec3(0, height / 2.f, 0)),
            .uv       = {},
        });
    }

    return vertices;
}

std::vector<uint32_t> Cylinder::generateIndices(const uint32_t segments) noexcept
{
    std::vector<uint32_t> indices;

    for (auto i = 0; i < segments; i++)
    {
        // Bottom face
        indices.push_back(0);
        indices.push_back(2 + i * 4);
        indices.push_back(i == segments - 1 ? 2 : 2 + (i + 1) * 4);

        // Top face
        indices.push_back(1);
        indices.push_back(i == segments - 1 ? 4 : (i + 2) * 4);
        indices.push_back((i + 1) * 4);

        // Side faces
        indices.push_back(3 + i * 4);
        indices.push_back(i == segments - 1 ? 5 : 5 + (i + 1) * 4);
        indices.push_back(i == segments - 1 ? 3 : 3 + (i + 1) * 4);

        indices.push_back(i == segments - 1 ? 5 : 5 + (i + 1) * 4);
        indices.push_back(3 + i * 4);
        indices.push_back(5 + i * 4);
    }

    return indices;
}
