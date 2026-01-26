#pragma once

#include <ranges>

namespace nbl
{
    /**
     * std::views::enumerate (C++23) is missing from clang libc++
     * https://en.cppreference.com/w/cpp/compiler_support/23.html#cpp_lib_ranges_enumerate_202302L
     */
    struct __enumerate
    {
        #ifdef _LIBCPP_VERSION
        template <std::ranges::viewable_range _Range>
        constexpr auto operator()(_Range&& range) const noexcept {
            return std::ranges::zip_view(
                std::ranges::iota_view<std::size_t>(0),
                range);
        };
        #else
        constexpr auto operator()(_Range&& range) const noexcept
        {
            return std::views::enumerate(range);
        }
        #endif
    };

    inline constexpr auto enumerate = __enumerate{};
}
