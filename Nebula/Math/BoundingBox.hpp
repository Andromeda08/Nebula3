#pragma once

#include <chrono>
#include <limits>
#include <print>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>

#include <xmmintrin.h>

[[nodiscard]] inline std::string toString(const glm::vec3& x)
{
    return std::format("[{}, {}, {}]", x[0], x[1], x[2]);
}

[[nodiscard]] inline glm::vec3 cwiseMin_simd(const glm::vec3& x, const glm::vec3& y)
{
    const __m128 _x = _mm_loadu_ps(glm::value_ptr(x));
    const __m128 _y = _mm_loadu_ps(glm::value_ptr(y));

    float* result = new float[4];
    _mm_storeu_ps(result, _mm_min_ps(_x, _y));

    return { result[0], result[1], result[2] };
}

[[nodiscard]] inline glm::vec3 cwiseMin(const glm::vec3& x, const glm::vec3& y)
{
    return {
        glm::min(x[0], y[0]),
        glm::min(x[1], y[1]),
        glm::min(x[2], y[2]),
    };
}

inline void test_cwiseMin()
{
    glm::vec3 x = { 1.0f, -5.0f, 2.5f };
    glm::vec3 y = { 0.1f, -4.5f, 2.0f };

    glm::vec3 expected = { 0.1f, -5.0f, 2.0f };

    const auto t1s = std::chrono::high_resolution_clock::now();
    const auto r1 = cwiseMin(x, y);
    const auto t1e = std::chrono::high_resolution_clock::now();

    auto eq1 = glm::epsilonEqual(r1, expected, glm::epsilon<float>());
    auto b1 = eq1.x && eq1.y && eq1.z;

    const auto t2s = std::chrono::high_resolution_clock::now();
    const auto r2 = cwiseMin_simd(x, y);
    const auto t2e = std::chrono::high_resolution_clock::now();

    auto eq2 = glm::epsilonEqual(r2, expected, glm::epsilon<float>());
    auto b2 = eq2.x && eq2.y && eq2.z;

    std::println("[Test : CWise min Test, expected={}]", toString(expected));
    std::println("Without : {} > {} [t={}]", toString(r1), b1 ? "Passed" : "Failed", std::chrono::microseconds((t1e - t1s).count()));
    std::println("With    : {} > {} [t={}]", toString(r2), b2 ? "Passed" : "Failed", std::chrono::microseconds((t2e - t2s).count()));
}

struct BoundingBox
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::infinity());
    glm::vec3 max = glm::vec3(-1.0f * std::numeric_limits<float>::infinity());

    BoundingBox& expandBy(const glm::vec3 point)
    {
        // const __m128 _min = _mm_loadu_ps(glm::value_ptr(min));
        // const __m128 _max = _mm_loadu_ps(glm::value_ptr(max));
        // const __m128 x    = _mm_loadu_ps(glm::value_ptr(point));
        //
        // _mm_storeu_ps(glm::value_ptr(min), _mm_min_ps(_min, x));
        // _mm_storeu_ps(glm::value_ptr(max), _mm_max_ps(_max, x));

        return *this;
    }
};
