#pragma once

#include <array>
#include <set>
#include <vector>

#include "Core/Types.hpp"
#include "Types/Light.hpp"

namespace RHI
{
    class Buffer;
    class VulkanRHI;
}

class LightSystem
{
public:
    constexpr static std::size_t sMaxLights = 100;

    explicit LightSystem(const SPtr<RHI::VulkanRHI>& rhi, const std::vector<Light>& initialLights = {});

    /**
     * Add a new Light (deferred data upload)
     * @param light New light parameters
     * @return New light index
     */
    uint64_t addLight(const Light& light) noexcept;

    /**
     * Gather and upload the queued changes to the GPU-side data buffer.
     */
    void upload() noexcept;

    [[nodiscard]] uint64_t getCount() const noexcept;

    [[nodiscard]] const SPtr<RHI::Buffer>& getDataBuffer() const noexcept;

    void queueUpdate(int32_t lightIndex) noexcept;

private:
    [[nodiscard]] std::set<uint64_t> getValidIndices() const noexcept;

    [[nodiscard]] std::vector<Light*> getValidLights() noexcept;

    friend class SceneInfoComponent;

    std::vector<std::size_t>        mUploadQueue;

    std::array<bool,  sMaxLights>   mValidity = {};
    std::array<Light, sMaxLights>   mLights = {};

    SPtr<RHI::Buffer>               mLightsBuffer;
    SPtr<RHI::VulkanRHI>            mRHI;
};
