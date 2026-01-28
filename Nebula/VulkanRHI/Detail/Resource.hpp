#pragma once

#include <string>
#include <string_view>
#include "Core/Types.hpp"
#include "VulkanRHI/Allocation.hpp"
#include "VulkanRHI/Device.hpp"

namespace RHI
{
    /**
     * RHI Resource base class
     * - All resources must have a reference to Device and a memory Allocation
     * - Optionally the debug label can be set
     */
    class Resource
    {
    public:
        virtual ~Resource() = default;

        explicit Resource(const SPtr<Device>& pDevice, const std::optional<SPtr<Allocation>>& pAllocation = std::nullopt);

        void setLabel(const std::string_view label) noexcept
        {
            mLabel = label;
        }

        void setAllocation(const SPtr<Allocation>& pAllocation) noexcept;

        template <class T>
        [[nodiscard]] T* as()
        {
            static_assert(std::is_base_of_v<Resource, T>, "Template parameter T must be a valid RHI Resource type!");
            return dynamic_cast<T*>(this);
        }

        [[nodiscard]] int32_t getId() const noexcept;

    protected:
        SPtr<Device>     mDevice;
        SPtr<Allocation> mAllocation;
        std::string      mLabel;

    private:
        const int32_t mId;

        static int32_t sIdSequence;
        static int32_t getNextId() noexcept;
    };
}
