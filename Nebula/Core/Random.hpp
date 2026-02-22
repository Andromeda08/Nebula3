#pragma once

#define nbl_RND_GLM

#include <limits>
#include <random>

#ifdef nbl_RND_GLM
#include <glm/glm.hpp>
#endif

class Random
{
public:
    template <class T>
    requires std::is_integral_v<T>
    [[nodiscard]] static T get(T from = std::numeric_limits<T>::min(), T to = std::numeric_limits<T>::max()) noexcept
    {
        return std::uniform_int_distribution{from, to}(Random::getInstance().mEngine);
    }

    template <class T>
    requires std::is_floating_point_v<T>
    [[nodiscard]] static T get(T from = T{0}, T to = T{1}) noexcept
    {
        return std::uniform_real_distribution{from, to}(Random::getInstance().mEngine);
    }

    [[nodiscard]] static float unit() noexcept
    {
        return std::uniform_real_distribution<float>{0.0f, 1.0f}(Random::getInstance().mEngine);
    }

    #ifdef nbl_RND_GLM

    /**
     * Random, normalized vector on the unit sphere.
     * @note (normal dist.)
     */
    template <class vec, typename V = vec::value_type>
    [[nodiscard]] static vec getUnitVector() noexcept
    {
        std::normal_distribution<V> dist{V{0}, V{1}};
        vec result;
        for (auto i = 0; i < vec::length(); i++)
        {
            result[i] = dist(Random::getInstance().mEngine);
        }
        return glm::normalize(result);
    }

    template <class vec, typename V = vec::value_type>
    [[nodiscard]] static vec getVector(V from = V{0}, V to = V{1}) noexcept
    {
        vec result;
        for (auto i = 0; i < vec::length(); i++)
        {
            result[i] = Random::get(from, to);
        }
        return result;
    }

    [[nodiscard]] static glm::vec4 getColor() noexcept
    {
        return { unit(), unit(), unit(), 1.0f };
    }

    #endif

private:
    static Random& getInstance() noexcept
    {
        static Random sInstance;
        return sInstance;
    }

    #ifdef NDEBUG
    Random() : mEngine(std::random_device{}()) {}
    #else
    Random() = default;
    #endif

    std::mt19937 mEngine;
};
