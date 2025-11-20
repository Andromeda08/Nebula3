#pragma once

#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Utility.hpp"

struct Gradient
{
    struct Key
    {
        glm::vec3 color { 1.0f };
        float     t {0.0f};
    };

    Gradient& clampColors(const bool value) noexcept
    {
        mClampColors = value;
        return *this;
    }

    Gradient& add(const glm::vec3& color, const float t) noexcept
    {
        if (const auto _t = std::ranges::find_if(mKeys, [&](const Key& k){ return k.t == t; });
            _t != std::end(mKeys))
        {
            Nbl_WARNING(fmt::format("Gradient already has a key at [t={}]", t));
            return *this;
        }

        mKeys.push_back({
            .color = mClampColors ? glm::clamp(color, 0.0f, 1.0f) : color,
            .t = glm::clamp(t, 0.0f, 1.0f),
        });
        std::ranges::sort(mKeys, [](const Key& a, const Key& b){ return a.t < b.t; });

        return *this;
    }

    Gradient& with(const glm::vec3& color, const float t) noexcept
    {
        return add(color, t);
    }

    Gradient& from(const glm::vec3& color) noexcept
    {
        add(color, 0.0f);
        return *this;
    }

    Gradient& to(const glm::vec3& color) noexcept
    {
        add(color, 1.0f);
        return *this;
    }

    glm::vec3 sample(const float t) const noexcept
    {
        if (mKeys.size() < 2)
        {
            return glm::vec3(0.0f);
        }

        size_t start = 0, end = 1;
        for (size_t i = 0; i < mKeys.size() - 1; i++)
        {
            if (t >= mKeys[i].t && t <= mKeys[i + 1].t)
            {
                start = i;
                end = i + 1;
                break;
            }
        }

        const auto& a = glm::hsvColor(mKeys[start].color);
        const auto& b = glm::hsvColor(mKeys[end].color);

        const float t_prime = (t - mKeys[start].t) / (mKeys[end].t - mKeys[start].t);

        // Hue interpolation
        float d = b.x - a.x;
        if (d >  180.0f) d -= 360.0f;
        if (d < -180.0f) d += 360.0f;

        float hue = a.x + t_prime * d;

        if (hue <  0.0f)   hue += 360.0f;
        if (hue >= 360.0f) hue -= 360.0f;

        // Linear for saturation and value
        const float sat = glm::mix(a.y, b.y, t_prime);
        const float val = glm::mix(a.z, b.z, t_prime);

        return glm::rgbColor(glm::vec3(hue, sat, val));
    }

    Gradient& normalizeValues() noexcept
    {
        for (std::size_t i = 0; i < mKeys.size(); i++)
        {
            auto& c = mKeys[i].color;
            c = glm::vec3(c.x / 255.0f, c.y / 255.0f, c.z / 255.0f);
        }
        return *this;
    }

private:
    std::vector<Key> mKeys;
    bool             mClampColors {true};
};
