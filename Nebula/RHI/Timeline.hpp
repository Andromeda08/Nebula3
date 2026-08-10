#pragma once

#include "Core/Util.hpp"

namespace RHI
{
    class ICommandList;

    class ITimelineSync
    {
    public:
        virtual ~ITimelineSync() = default;

        virtual uint64_t getSignaledValue() const = 0;
        virtual bool hostWait(uint64_t value, uint64_t timeout) const = 0;
        virtual void hostSignal(uint64_t value) const = 0;
    };

    struct TimelineSyncPoint
    {
        ITimelineSync* pSync = nullptr;
        uint64_t       value = 0;
    };

    struct SubmitInfo
    {
        std::vector<ICommandList*>     commandLists;
        std::vector<TimelineSyncPoint> waits;
        std::vector<TimelineSyncPoint> signals;

        SubmitInfo& addCommandList(ICommandList* pList)
        {
            commandLists.push_back(pList);
            return *this;
        }

        SubmitInfo& addWait(ITimelineSync* pSync, const uint64_t value)
        {
            waits.push_back({
                .pSync = pSync,
                .value = value,
            });
            return *this;
        }

        SubmitInfo& addSignal(ITimelineSync* pSync, const uint64_t value)
        {
            signals.push_back({
                .pSync = pSync,
                .value = value,
            });
            return *this;
        }
    };

    class Timeline
    {
    public:
        nbl_DisableCopy(Timeline);

        explicit Timeline(UPtr<ITimelineSync> sync) : mSync(std::move(sync)) {}

        [[nodiscard]] uint64_t getNextValue() noexcept
        {
            return ++mLast;
        }

        [[nodiscard]] uint64_t getLastValue() const noexcept
        {
            return mLast;
        }

        [[nodiscard]] bool isComplete(const uint64_t value) const
        {
            return mSync->getSignaledValue() >= value;
        }

        [[nodiscard]] bool hostWait(const uint64_t value, const uint64_t timeout = std::numeric_limits<uint64_t>::max()) const
        {
            return mSync->hostWait(value, timeout);
        }

        [[nodiscard]] ITimelineSync* getSync() const noexcept
        {
            return mSync.get();
        }

        [[nodiscard]] TimelineSyncPoint makePoint(const uint64_t value) const noexcept
        {
            return{
                .pSync = mSync.get(),
                .value = value,
            };
        }

    private:
        UPtr<ITimelineSync> mSync;
        uint64_t            mLast = 0;
    };
}
