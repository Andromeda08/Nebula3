#include "CIFGeometry.hpp"

#include <numbers>

namespace geo
{
    void Data::setPosition(const glm::vec3& pos)
    {
        for (auto& e : vertices) {
            e += pos;
        }
    }

    void Data::setRotation(const glm::vec3& axis, float angle)
    {
        for (auto& e : vertices) {
            e = glm::rotate(e, angle, axis);
        }
    }

    BondTransform calcBondTransforms(const glm::vec3& atom1, const glm::vec3& atom2)
    {
        BondTransform bt;

        glm::vec3 dir = atom2 - atom1;
        bt.dist       = glm::length(dir);

        glm::vec3 v1(0.f, 1.f, 0.f);
        glm::vec3 v2    = dir / bt.dist;
        float     angle = glm::acos(glm::dot(v1, v2));

        bt.axis     = glm::cross(v1, v2);
        bt.angle    = 180.f * angle / glm::pi<float>();
        bt.position = (atom1 + atom2) / 2.f;

        return bt;
    }

    std::vector<glm::vec3> generateSphereVertices(const int32_t stackCount, const int32_t sectorCount, const float radius)
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
        return vertices;
    }

    std::vector<uint32_t> generateSphereIndices(const int32_t stackCount, const int32_t sectorCount)
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

    Data createSphereData2(const float r, const int32_t lat, const int32_t lng)
    {
        return {
            .vertices = generateSphereVertices(lat, lng, r),
            .normals = {},
            .indices = generateSphereIndices(lat, lng)
        };
    }

    Data createSphereData(float radius, uint32_t latSeg, uint32_t lngSeg)
    {
        Data data;

        // Center
        data.vertices.emplace_back(0.f, radius, 0.f);
        data.vertices.emplace_back(0.f, -radius, 0.f);
        data.normals.emplace_back(0.f, 1.f, 0.f);
        data.normals.emplace_back(0.f, -1.f, 0.f);

        // First/Last rings
        for (auto j = 0; j < lngSeg; j++) {
            data.indices.push_back(0);
            data.indices.push_back(j == lngSeg - 1 ? 2 : (j + 3));
            data.indices.push_back(2 + j);

            data.indices.push_back(2 + (latSeg - 2) * lngSeg + j);
            data.indices.push_back(j == lngSeg - 1 ? 2 + (latSeg - 2) * lngSeg : 2 + (latSeg - 2) * lngSeg + j + 1);
            data.indices.push_back(1);
        }

        // Rest of rings
        for (auto i = 1; i < latSeg; i++) {
            float angleV = static_cast<float>(i) * glm::pi<float>() / static_cast<float>(latSeg);
            for (auto j = 0; j < lngSeg; j++) {
                float     angleH   = static_cast<float>(j) * glm::two_pi<float>() / static_cast<float>(lngSeg);
                glm::vec3 position = {
                    radius * glm::sin(angleV) * glm::cos(angleH),
                    radius * glm::cos(angleV),
                    radius * glm::sin(angleV) * glm::sin(angleH)
                };
                data.vertices.push_back(position);
                data.normals.push_back(glm::normalize(position));

                if (i == 1) continue;

                data.indices.push_back(2 + (i - 1) * lngSeg + j);
                data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 2) * lngSeg : 2 + (i - 2) * lngSeg + j + 1);
                data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 1) * lngSeg : 2 + (i - 1) * lngSeg + j + 1);

                data.indices.push_back(j == lngSeg - 1 ? 2 + (i - 2) * lngSeg : 2 + (i - 2) * lngSeg + j + 1);
                data.indices.push_back(2 + (i - 1) * lngSeg + j);
                data.indices.push_back(2 + (i - 2) * lngSeg + j);
            }
        }

        return data;
    }

    Data createCylinderData(uint32_t seg, float radius, float height)
    {
        Data data;

        // Center
        data.vertices.emplace_back(0.f, -height / 2.f, 0.f);
        data.vertices.emplace_back(0.f, height / 2.f, 0.f);
        data.normals.emplace_back(0.f, -1.f, 0.f);
        data.normals.emplace_back(0.f, 1.f, 0.f);

        // Circle
        float step = glm::two_pi<float>() / static_cast<float>(seg);
        for (auto i = 0; i < seg; i++) {
            glm::vec3 position = {
                glm::cos(i * step) * radius,
                -height / 2.f,
                glm::sin(i * step) * radius
            };

            // Bottom ring
            data.vertices.push_back(position);
            data.vertices.push_back(position);
            data.normals.emplace_back(0.f, -1.f, 0.f);
            data.normals.push_back(glm::normalize(position - glm::vec3(0, -height / 2.f, 0)));

            // Top ring
            position.y = height / 2.f;
            data.vertices.push_back(position);
            data.vertices.push_back(position);
            data.normals.emplace_back(0.f, 1.f, 0.f);
            data.normals.push_back(glm::normalize(position - glm::vec3(0, height / 2.f, 0)));

            // Bottom face
            data.indices.push_back(0);
            data.indices.push_back(2 + i * 4);
            data.indices.push_back(i == seg - 1 ? 2 : 2 + (i + 1) * 4);

            // Top face
            data.indices.push_back(1);
            data.indices.push_back(i == seg - 1 ? 4 : (i + 2) * 4);
            data.indices.push_back((i + 1) * 4);

            // Side faces
            data.indices.push_back(3 + i * 4);
            data.indices.push_back(i == seg - 1 ? 5 : 5 + (i + 1) * 4);
            data.indices.push_back(i == seg - 1 ? 3 : 3 + (i + 1) * 4);

            data.indices.push_back(i == seg - 1 ? 5 : 5 + (i + 1) * 4);
            data.indices.push_back(3 + i * 4);
            data.indices.push_back(5 + i * 4);
        }

        return data;
    }
}
