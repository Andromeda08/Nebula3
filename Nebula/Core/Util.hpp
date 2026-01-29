#pragma once

#include <format>
#include <print>
#include <string_view>
#include <spdlog/spdlog.h>

template <class... Args>
[[noreturn]] void exitWithError(spdlog::format_string_t<Args...> fmt, Args &&...args) noexcept
{
    spdlog::set_level(spdlog::level::critical);
    spdlog::critical(fmt, std::forward<Args>(args)...);

    // "portable" __debugbreak() / __builtin_debugtrap()
    #if defined(__x86_64__) || defined(_M_X64)
    asm("int 3");
    #else
    // Breakpoint for arm platforms
    asm("brk #0xF000");
    #endif

    std::exit(EXIT_FAILURE);
}

template <class... Args>
void exitOnAssert(const bool condition, spdlog::format_string_t<Args...> fmt, Args&&... args) noexcept
{
    [[unlikely]] if (!condition)
    {
        exitWithError(fmt, std::forward<Args>(args)...);
    }
}


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
