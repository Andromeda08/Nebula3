#pragma once

#include <limits>
#include <glm/glm.hpp>

namespace nbl
{
    struct BoundingSphere
    {
        glm::vec3   center = glm::vec3(0.0f);
        float       radius = 1.0f;
    };

    // 3-dimensional bounding box data structure
    class BoundingBox
    {
    public:
        BoundingBox() = default;

        explicit BoundingBox(const glm::vec3& point);

        BoundingBox(const glm::vec3& min, const glm::vec3& max);

        [[nodiscard]] BoundingBox getTransformed(const glm::mat4& tr) const noexcept;

        void reset() noexcept
        {
            mMin = glm::vec3(std::numeric_limits<float>::max());
            mMax = glm::vec3(std::numeric_limits<float>::lowest());
        }

        // Check whether this is a valid bounding box.
        [[nodiscard]] bool isValid() const noexcept;

        // Check whether this bounding box has collapsed to a single point
        [[nodiscard]] bool isPoint() const noexcept;

        // Check whether this bounding box has any associated volume
        [[nodiscard]] bool hasVolume() const noexcept;

        // Get the bounding sphere defined by center and radius for this bounding box.
        [[nodiscard]] BoundingSphere getBoundingSphere() const noexcept;

        // Expand the bounding box by a Point.
        BoundingBox& expandBy(const glm::vec3& point) noexcept;

        // Expand the bounding box by another bounding box.
        BoundingBox& expandBy(const BoundingBox& boundingBox) noexcept;

        // Check if the given point is within the bounding box.
        [[nodiscard]] bool contains(const glm::vec3& point, bool strict = false) const noexcept;

        // Check if the given bounding box is contained within the bounding box.
        [[nodiscard]] bool contains(const BoundingBox& boundingBox, bool strict = false) const noexcept;

        [[nodiscard]] const glm::vec3& getMin() const noexcept;

        [[nodiscard]] const glm::vec3& getMax() const noexcept;

        bool operator==(const BoundingBox& other) const noexcept;

    private:
        glm::vec3 mMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 mMax = glm::vec3(std::numeric_limits<float>::lowest());
    };
}
