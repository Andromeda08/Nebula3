#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>

#include <spdlog/spdlog.h>

#if defined(_MSC_VER)
    #define sunflower_DebugBreak() __debugbreak()
#elif __has_builtin(__builtin_debugtrap)
    #define sunflower_DebugBreak() __builtin_debugtrap()
#else
    #define sunflower_DebugBreak() __builtin_trap()
#endif

// Build type
namespace sunflower::conf
{
    inline constexpr bool gIsDebug =
    #ifdef SUNFLOWER_DEBUG
        true;
    #else
        false;
    #endif

    inline constexpr bool gIsMoltenVk =
    #ifdef SUNFLOWER_MOLTEN_VK
        true;
    #else
        false;
    #endif

    inline constexpr bool gIsApple =
    #ifdef __APPLE__
        true;
    #else
        false;
    #endif

    // Configures the number of frames in flight
    constexpr uint64_t gFramesInFlight = 3;
}

// Smart pointer aliases
namespace sunflower
{
    template <class T> using UPtr = std::unique_ptr<T>;
    template <class T> using SPtr = std::shared_ptr<T>;

    template <class T, class... Args>
    [[nodiscard]] constexpr UPtr<T> makeUnique(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    [[nodiscard]] SPtr<T> makeShared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}

// Other types, utilities and aliases
namespace sunflower
{
    template <class T>
    using PerFrameArray = std::array<T, conf::gFramesInFlight>;

    using String = std::string;

    template <class T>
    using Option = std::optional<T>;

    template <class... Args>
    [[noreturn]] void exit(spdlog::format_string_t<Args...> fmt, Args&&... args) noexcept
    {
        if constexpr (conf::gIsDebug)
        {
            try
            {
                spdlog::critical(fmt, std::forward<Args>(args)...);
                spdlog::default_logger()->flush();
            }
            catch (...) {}

            sunflower_DebugBreak();
        }
        std::abort();
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

    #pragma region "std::views::enumerate"

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
        template <std::ranges::viewable_range _Range>
        constexpr auto operator()(_Range&& range) const noexcept
        {
            return std::views::enumerate(range);
        }
        #endif
    };

    inline constexpr auto enumerate = __enumerate{};

    #pragma endregion

    struct Size
    {
        uint32_t width  = 0;
        uint32_t height = 0;
        uint32_t depth  = 0;
    };

    enum class VendorID : uint32_t
    {
        AMD     = 0x1002,
        Apple   = 0x106B,
        Intel   = 0x8086,
        NVIDIA  = 0x10DE,
        Other   = 0,
    };

    inline constexpr std::pair<VendorID, std::string_view> gVendors[] = {
        { VendorID::AMD,    "AMD"    },
        { VendorID::Apple,  "Apple"  },
        { VendorID::Intel,  "Intel"  },
        { VendorID::NVIDIA, "NVIDIA" },
    };

    [[nodiscard]] constexpr VendorID toVendorID(const uint32_t id)
    {
        const auto eId = static_cast<VendorID>(id);
        return contains(gVendors | std::views::keys, eId)
            ? eId : VendorID::Other;
    }

    enum class QueueType
    {
        Compute,
        Graphics,
        Transfer,
    };
}

#define sunflower_INTERFACE(T)          \
    T() = default;                      \
    T(const T&) = delete;               \
    T& operator=(const T&) = delete;    \
    virtual ~T() = default;

#define sunflower_DisableCopy(T) \
    T(const T&) = delete;               \
    T& operator=(const T&) = delete;    \
    T(const T&&) = delete;              \
    T& operator=(const T&&) = delete;

#define sunflower_Create(T, Ptr)                                                    \
    private: explicit T(const T##CreateInfo& createInfo);                           \
    public: [[nodiscard]] static Ptr<T> create(const T##CreateInfo& createInfo) {   \
        return Ptr<T>(new T(createInfo));                                           \
    }