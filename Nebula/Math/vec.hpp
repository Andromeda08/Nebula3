#pragma once

// Check for SIMD intrinsic availability
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define nbl_Neon
    #include <arm_neon.h>
#elif defined(__AVX__)
    #define nbl_AVX
    #include <xmmintrin.h>
#endif

#include <format>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nbl
{
    /**
     * Convert glm vectors of any size to a string.
     * @return [x, y, ...] formatted string
     */
    template <class vec>
    [[nodiscard]] std::string vec_toString(const vec& v)
    {
        std::stringstream sstr;
        sstr << "[";
        for (auto i = 0; i < vec::length(); ++i)
        {
            if (i != 0)
            {
                sstr << ", ";
            }
            sstr << v[i];
        }
        sstr << "]";
        return sstr.str();
    }

    /**
     * Epsilon Equal for glm vectors of any size
     * @return True if all components eps. equal.
     */
    template <class vec>
    [[nodiscard]] bool vec_epsilonEqual(const vec& a, const vec& b)
    {
        const auto eq = glm::epsilonEqual(a, b, glm::epsilon<float>());
        bool result = true;
        for (auto i = 0; i < vec::length(); ++i)
        {
            result &= eq[i];
        }
        return result;
    }

    struct MinMaxResult
    {
        glm::vec4 min = glm::vec4(std::numeric_limits<float>::max());
        glm::vec4 max = glm::vec4(std::numeric_limits<float>::lowest());

        [[nodiscard]] std::string toString() const noexcept
        {
            return std::format("[min={}, max={}]", vec_toString(min), vec_toString(max));
        }

        [[nodiscard]] bool operator==(const MinMaxResult& other) const noexcept
        {
            return vec_epsilonEqual(this->min, other.min)
                && vec_epsilonEqual(this->max, other.max);
        }
    };

    /**
     * Scalar implementation of cwiseMinMax_x4
     */
    inline void cwiseMinMax_x4_vanilla(MinMaxResult* pCurrent, const glm::vec4* v) noexcept
    {
        // Apply an operation on the values across "v" for a single component
        #define nbl_Component_Op(op, c) \
            glm::op(glm::op(v[0].c, v[1].c), glm::op(v[2].c, v[3].c))
        #define nbl_Component_Min(c) nbl_Component_Op(min, c)
        #define nbl_Component_Max(c) nbl_Component_Op(max, c)

        pCurrent->min = glm::min(pCurrent->min, glm::vec4(
            nbl_Component_Min(x), nbl_Component_Min(y), nbl_Component_Min(z), nbl_Component_Min(w)
        ));

        pCurrent->max = glm::max(pCurrent->max, glm::vec4(
            nbl_Component_Max(x), nbl_Component_Max(y), nbl_Component_Max(z), nbl_Component_Max(w)
        ));

        #undef nbl_Component_Op
        #undef nbl_Component_Min
        #undef nbl_Component_Max
    }

    #ifdef nbl_Neon
    /**
     * Arm Neon implementation of cwiseMinMax_x4
     */
    inline void cwiseMinMax_x4_neon(MinMaxResult* pCurrent, const glm::vec4* vectors) noexcept
    {
        const float32x4x4_t v = vld1q_f32_x4(reinterpret_cast<const float*>(vectors));

        const float32x4_t min = vminq_f32(vminq_f32(v.val[0], v.val[1]), vminq_f32(v.val[2], v.val[3]));
        const float32x4_t max = vmaxq_f32(vmaxq_f32(v.val[0], v.val[1]), vmaxq_f32(v.val[2], v.val[3]));

        vst1q_f32(glm::value_ptr(pCurrent->min), vminq_f32(vld1q_f32(glm::value_ptr(pCurrent->min)), min));
        vst1q_f32(glm::value_ptr(pCurrent->max), vmaxq_f32(vld1q_f32(glm::value_ptr(pCurrent->max)), max));
    }
    #endif

    /**
     * Compute the component-wise min and max values between the given next 4 vectors and
     * update the tracked current values. Vectorized when available.
     * @param pCurrent Tracked min and max values
     * @param vectors Incoming vectors to process.
     */
    inline void cwiseMinMax_x4(MinMaxResult* pCurrent, const glm::vec4* vectors) noexcept
    {
        #if defined(nbl_Neon)
        cwiseMinMax_x4_neon(pCurrent, vectors);
        #elif defined(nbl_AVX)
        // cwiseMinMax_x4_avx2(pCurrent, vectors);
        cwiseMinMax_x4_vanilla(pCurrent, vectors);
        #else
        cwiseMinMax_x4_vanilla(pCurrent, vectors);
        #endif
    }

    /**
     * Compute the component-wise min and max values across the input data.
     * @param data
     */
    [[nodiscard]] inline MinMaxResult cwiseMinMax(const std::vector<glm::vec4>& data) noexcept
    {
        MinMaxResult result = {};

        const size_t count     = data.size();
        const size_t simdCount = count & ~static_cast<size_t>(3);

        size_t i = 0;
        for (; i < simdCount; i += 4)
        {
            cwiseMinMax_x4(&result, data.data() + i);
        }

        for (; i < count; ++i)
        {
            result.min = glm::min(result.min, data[i]);
            result.max = glm::max(result.max, data[i]);
        }

        return result;
    }
}
