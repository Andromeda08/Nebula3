#include "BoundingBox.hpp"

namespace nbl
{
    BoundingBox::BoundingBox(const glm::vec3& point)
    : mMin(point)
    , mMax(point)
    {
    }

    BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max)
    : mMin(min)
    , mMax(max)
    {
    }

    BoundingBox BoundingBox::getTransformed(const glm::mat4& tr) const noexcept
    {
        const auto center = (mMax + mMin) * 0.5f;
        const auto halfSize = center - mMin;

        const auto scaleRotate = glm::mat3(
            glm::abs(glm::vec3(tr[0])),
            glm::abs(glm::vec3(tr[1])),
            glm::abs(glm::vec3(tr[2])));

        const auto newCenter = glm::vec3(tr * glm::vec4(center, 1.0f));
        const auto newHalf   = scaleRotate * halfSize;

        return BoundingBox(newCenter - newHalf, newCenter + newHalf);
    }

    bool BoundingBox::isValid() const noexcept
    {
        return glm::all(glm::lessThanEqual(mMin, mMax));
    }

    bool BoundingBox::isPoint() const noexcept
    {
        return glm::all(glm::equal(mMin, mMax));
    }

    bool BoundingBox::hasVolume() const noexcept
    {
        return glm::all(glm::lessThan(mMin, mMax));
    }

    BoundingSphere BoundingBox::getBoundingSphere() const noexcept
    {
        const auto center = (mMax + mMin) * 0.5f;
        const auto extent = (mMax - mMin) * 0.5f;
        return {
            .center = center,
            .radius = glm::length(extent),
        };
    }

    BoundingBox& BoundingBox::expandBy(const glm::vec3& point) noexcept
    {
        mMin = glm::min(mMin, point);
        mMax = glm::max(mMax, point);
        return *this;
    }

    BoundingBox& BoundingBox::expandBy(const BoundingBox& boundingBox) noexcept
    {
        mMin = glm::min(mMin, boundingBox.mMin);
        mMax = glm::max(mMax, boundingBox.mMax);
        return *this;
    }

    bool BoundingBox::contains(const glm::vec3& point, const bool strict) const noexcept
    {
        if (strict)
        {
            return glm::all(glm::greaterThan(point, mMin))
                && glm::all(glm::lessThan(point, mMax));
        }
        else
        {
            return glm::all(glm::greaterThanEqual(point, mMin))
                && glm::all(glm::lessThanEqual(point, mMax));
        }
    }

    bool BoundingBox::contains(const BoundingBox& boundingBox, const bool strict) const noexcept
    {
        if (strict)
        {
            return glm::all(glm::greaterThan(boundingBox.mMin, mMin))
                && glm::all(glm::lessThan(boundingBox.mMax, mMax));
        }
        else
        {
            return glm::all(glm::greaterThanEqual(boundingBox.mMin, mMin))
                && glm::all(glm::lessThanEqual(boundingBox.mMax, mMax));
        }
    }

    const glm::vec3& BoundingBox::getMin() const noexcept
    {
        return mMin;
    }

    const glm::vec3& BoundingBox::getMax() const noexcept
    {
        return mMax;
    }

    bool BoundingBox::operator==(const BoundingBox& other) const noexcept
    {
        return glm::all(glm::equal(mMin, other.mMin))
            && glm::all(glm::equal(mMax, other.mMax));
    }
}
