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
    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        virtual void begin() = 0;
        virtual void end() = 0;
    };

    class ICommandPool
    {
    public:
        virtual ~ICommandPool() = default;

        virtual ICommandList* allocate() = 0;
        virtual void free(ICommandList* pCommandList) = 0;
        virtual void reset() const = 0;
    };

    class ICommandQueue
    {
    public:
        virtual ~ICommandQueue() = default;

        virtual void submit(const SubmitInfo& submitInfo) const = 0;

        [[nodiscard]] virtual Timeline* getTimeline() const = 0;

        [[nodiscard]] virtual bool isComplete(uint64_t value) const = 0;

        virtual void signal(uint64_t value) const = 0;

        virtual void waitIdle() const = 0;

        virtual void immediate(const std::function<void(ICommandList*)>& fn) const = 0;

        [[nodiscard]] virtual SPtr<ICommandPool> createCommandPool() const = 0;
    };

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        [[nodiscard]] virtual const std::string& getDeviceName() const = 0;
    };

    class DynamicRHI
    {
    public:
        virtual ~DynamicRHI() = default;
    };
}
