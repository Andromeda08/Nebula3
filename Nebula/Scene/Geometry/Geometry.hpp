#pragma once

#include <string>
#include <vector>
#include "Scene/Types/Vertex.hpp"

struct GeometryCreateInfo
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::string             name;
};

class Geometry
{
public:
    explicit Geometry(const GeometryCreateInfo& createInfo);

    virtual ~Geometry() = default;

    [[nodiscard]] const std::vector<Vertex>& getVertices() const noexcept
    {
        return mVertices;
    }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const noexcept
    {
        return mIndices;
    }

    [[deprecated("Use getVertexCount instead")]]
    [[nodiscard]] uint32_t vertexCount() const noexcept
    {
        return mVertexCount;
    }

    [[nodiscard]] uint32_t getVertexCount() const noexcept
    {
        return mVertexCount;
    }

    [[deprecated("Use getIndexCount instead")]]
    [[nodiscard]] uint32_t indexCount() const noexcept
    {
        return mIndexCount;
    }


    [[nodiscard]] uint32_t getIndexCount() const noexcept
    {
        return mIndexCount;
    }

    [[nodiscard]] const std::string& getName() const noexcept
    {
        return mName;
    }

protected:
    Geometry() = default;

    std::vector<Vertex>     mVertices    = {};
    uint32_t                mVertexCount = 0;
    std::vector<uint32_t>   mIndices     = {};
    uint32_t                mIndexCount  = 0;
    std::string             mName        = "Unknown Geometry";
};

// ============================
// Primitives
// ============================

class Cube final : public Geometry
{
public:
    constexpr static auto sName = "Cube";

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
    constexpr static auto sName = "Sphere";

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

class Cylinder final : public Geometry
{
public:
    constexpr static auto sName = "Cylinder";

    struct Params
    {
        float    radius       = 1.0f;
        float    height       = 1.0f;
        uint32_t tesselation = 60;
    };

    explicit Cylinder(const Params& params);

    ~Cylinder() override = default;

private:
    static std::vector<Vertex> generateVertices(uint32_t segments, float radius, float height) noexcept;

    static std::vector<uint32_t> generateIndices(uint32_t segments) noexcept;
};
