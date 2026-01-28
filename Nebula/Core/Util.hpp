#pragma once

#include <format>
#include <print>
#include <string_view>

namespace util::debug
{
    inline void nblAssertImpl(
        const bool       condition,
        std::string_view message) noexcept
    {
        if (!condition)
        {
            std::println("Assertion failed: {}", message);
            #ifdef NDEBUG
                std::exit(EXIT_FAILURE);
            #else
                #if defined(_MSC_VER)
                __debugbreak();
                #elif defined(__GNUC__) || defined(__clang__)
                __builtin_trap();
                #else
                std::abort();
                #endif
            #endif
        }
    }
}

#define nbl_ASSERT(condition, ...) \
    ::util::debug::nblAssertImpl((condition), std::format(__VA_ARGS__))
