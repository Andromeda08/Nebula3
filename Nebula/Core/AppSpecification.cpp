#include "AppSpecification.hpp"

#include "Platform.hpp"

namespace detail
{
    Size2D getDefaultResolution() noexcept
    {
        return platform::isApple ? Resolution::w1600h900() : Resolution::w1920h1080();
    }
}
