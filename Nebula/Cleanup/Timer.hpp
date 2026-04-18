#pragma once

#include <chrono>

namespace nbl
{
    class Timer
    {
        using ClockType = std::chrono::high_resolution_clock;
    public:
        Timer& reset() noexcept;

        // Record starting time
        Timer& start() noexcept;

        // Returns elapsed time
        [[nodiscard]] float stop() noexcept;

    private:
        bool                    mReady = false;
        ClockType::time_point   mStart;
    };
}