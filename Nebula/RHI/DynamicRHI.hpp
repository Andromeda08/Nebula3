#pragma once

#include "Timeline.hpp"
#include "Core/Util.hpp"

// rhi_cast
namespace RHI
{
    template <class To, class From>
    requires std::derived_from<To, From>
    [[nodiscard]] To* rhi_cast(From* ptr) noexcept
    {
        #ifdef nbl_DEBUG
        auto* result = dynamic_cast<To*>(ptr);
        if (!result)
        {
            exitWithError("RHI type mismatch!");
        }
        return result;
        #else
        return static_cast<To*>(ptr);
        #endif
    }

    template <class To, class From>
    requires std::derived_from<To, From>
    [[nodiscard]] const To* rhi_cast(const From* ptr) noexcept
    {
        #ifdef nbl_DEBUG
        auto* result = dynamic_cast<const To*>(ptr);
        if (!result)
        {
            exitWithError("RHI type mismatch!");
        }
        return result;
        #else
        return static_cast<const To*>(ptr);
        #endif
    }

    template <class To, class From>
    requires std::derived_from<To, From>
    [[nodiscard]] SPtr<To> rhi_cast(const SPtr<From>& ptr) noexcept
    {
        #ifdef nbl_DEBUG
        auto result = std::dynamic_pointer_cast<To>(ptr);
        if (!result)
        {
            exitWithError("RHI type mismatch!");
        }
        return result;
        #else
        return std::static_pointer_cast<To>(ptr);
        #endif
    }
}

namespace RHI
{
    /**
     * Shared CommandList interface.
     */
    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        virtual void begin() = 0;
        virtual void end() = 0;
    };

    /**
     * Shared CommandPool interface.
     */
    class ICommandPool
    {
    public:
        virtual ~ICommandPool() = default;

        // Allocate a CommandList from this Pool
        virtual ICommandList* allocate() = 0;

        // Free a CommandList
        virtual void free(ICommandList* pCommandList) = 0;

        // Reset the Pool
        virtual void reset() const = 0;
    };

    /**
     * Shared CommandQueue interface.
     */
    class ICommandQueue
    {
    public:
        virtual ~ICommandQueue() = default;

        // Create a command pool for this queue to use for command list allocation.
        virtual SPtr<ICommandPool> createCommandPool() const = 0;

        // Submit work to the Queue.
        virtual void submit(const SubmitInfo& submitInfo) const = 0;

        // Check if the value counter has passed the given value.
        virtual bool isComplete(uint64_t value) const = 0;

        // Wait Idle by waiting for the next counter value.
        virtual void waitIdle() const = 0;

        // Record and submit a command list for immediate execution.
        virtual void immediate(const std::function<void(ICommandList*)>& fn) const = 0;

        // Get the underlying timeline used by the Queue.
        virtual Timeline* getTimeline() const = 0;
    };

    /**
     * Base class for backends.
     */
    class DynamicRHI
    {
    public:
        virtual ~DynamicRHI() = default;

        virtual ICommandQueue* getGraphicsQueue() const = 0;
    };
}
