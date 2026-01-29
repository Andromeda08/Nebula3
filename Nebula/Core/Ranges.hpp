#pragma once

#include <functional>
#include <ranges>
#include <sstream>
#include <string>

template <typename F, typename T>
concept UnaryPredicate = std::is_invocable_r_v<bool, F, const T&>;

template <std::ranges::input_range _Range>
[[nodiscard]] std::string join(_Range&& range, const std::string& delimiter = ", ") noexcept
{
    std::stringstream sstr;
    bool isFirst = true;
    for (const auto& elem : range)
    {
        if (!isFirst)
        {
            sstr << delimiter;
        }
        isFirst = false;
        sstr << elem;
    }
    return sstr.str();
}

template <std::ranges::input_range _Range>
[[nodiscard]] bool contains(_Range&& range, const std::ranges::range_value_t<_Range>& val) noexcept
{
    return std::ranges::find(range, val) != std::ranges::end(range);
}

template <std::ranges::input_range _Range, class Pred>
requires std::predicate<Pred, std::ranges::range_reference_t<_Range>>
[[nodiscard]] bool containsIf(_Range&& range, Pred&& pred) noexcept
{
    return std::ranges::find_if(range, std::forward<Pred>(pred)) != std::ranges::end(range);
}