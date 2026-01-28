#include "Resource.hpp"

#include "Core/Util.hpp"

namespace RHI
{
    int32_t Resource::sIdSequence = 0;

    Resource::Resource(const SPtr<Device>& pDevice, const std::optional<SPtr<Allocation>>& pAllocation)
    : mDevice(pDevice)
    , mAllocation(pAllocation.value_or(nullptr))
    , mLabel("Unknown Resource")
    , mId(getNextId())
    {
        nbl_ASSERT(pDevice != nullptr, "Resources must have a valid Device reference!");
    }

    void Resource::setAllocation(const SPtr<Allocation>& pAllocation) noexcept
    {
        mAllocation = pAllocation;
    }

    int32_t Resource::getId() const noexcept
    {
        return mId;
    }

    int32_t Resource::getNextId() noexcept
    {
        return sIdSequence++;
    }
}
