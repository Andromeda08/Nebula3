#pragma once

#include <string>
#include <vector>
#include "Scene/Types/Vertex.hpp"

struct GeometryCreateInfo
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
};

class Geometry
{
public:
    explicit Geometry(const GeometryCreateInfo& createInfo);

    virtual ~Geometry() = default;

    const std::vector<Vertex>& getVertices() const { return mVertices; }
    const std::vector<uint32_t>& getIndices() const { return mIndices; }

    uint32_t vertexCount() const { return mVertexCount; }
    uint32_t indexCount() const { return mIndexCount; }

protected:
    Geometry() = default;

    std::vector<Vertex>     mVertices    = {};
    uint32_t                mVertexCount = 0;
    std::vector<uint32_t>   mIndices     = {};
    uint32_t                mIndexCount  = 0;
    std::string             mName        = "Unknown Geometry";
};

class Cube final : public Geometry
{
public:
    struct Params
    {
        float scale {0.5f};
    };

    explicit Cube(const Params& params);

    ~Cube() override = default;

private:
    static std::vector<Vertex> generateVertices(float scale);

    static std::vector<Vertex>   sCubeVertices;
    static std::vector<uint32_t> sCubeIndices;
};

class Sphere final : public Geometry
{
public:
    struct Params
    {
        float    radius {1.0f};
        uint32_t tesselationX {60};
        uint32_t tesselationY {60};
    };

    explicit Sphere(const Params& params);

    ~Sphere() override = default;

private:
    static std::vector<Vertex> generateVertices(int32_t stackCount, int32_t sectorCount, float radius);

    static std::vector<uint32_t> generateIndices(int32_t stackCount, int32_t sectorCount);
};