#include "Timer.hpp"

#include <spdlog/spdlog.h>

namespace nbl
{
    Timer& Timer::reset() noexcept
    {
        mReady = false;
        mStart = ClockType::now();

        return *this;
    }

    Timer& Timer::start() noexcept
    {
        if (mReady)
        {
            spdlog::warn("Stopper is already running.");
            return *this;
        }

        mReady = true;
        mStart = ClockType::now();

        return *this;
    }

    float Timer::stop() noexcept
    {
        if (!mReady)
        {
            spdlog::warn("Stopper wasn't started.");
            return 0.0f;
        }

        mReady = false;

        const auto currentTime = ClockType::now();
        return std::chrono::duration<float>(currentTime - mStart).count();
    }
}
