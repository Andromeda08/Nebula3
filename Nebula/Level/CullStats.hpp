#pragma once

#include <cstdint>

#include "UserInterface/IComponent.hpp"

namespace nbl
{
    struct CullStats
    {
        /**
         * Create culling statistics from the parameters. Computes culled% and culled count.
         */
        static CullStats make(const uint32_t objCount, const uint32_t visible, const float ms) noexcept
        {
            return {
                .totalObjectCount = objCount,
                .culledCount      = objCount - visible,
                .visibleCount     = visible,
                .percent          = static_cast<float>(objCount - visible) / static_cast<float>(objCount),
                .cullTimeMs       = ms,
            };
        }

        uint32_t totalObjectCount   = 0;
        uint32_t culledCount        = 0;
        uint32_t visibleCount       = 0;
        float    percent            = 0;
        float    cullTimeMs         = 0.0f;
    };

    class CullStatsUI : public IComponent
    {
    public:
        explicit CullStatsUI(CullStats* pCullStats, bool* enableCulling);

        void draw() override;

    private:
        CullStats*  mCullStats;
        bool*       mEnableCulling;
    };
}
