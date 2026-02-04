#include "LightSystem.hpp"

#include <ranges>
#include <spdlog/spdlog.h>
#include "Core/Ranges.hpp"
#include "VulkanRHI/VulkanRHI.hpp"

LightSystem::LightSystem(const SPtr<RHI::VulkanRHI>& rhi, const std::vector<Light>& initialLights)
: mRHI(rhi)
{
    mLightsBuffer = mRHI->createBuffer({
        .size  = sMaxLights * sizeof(GPULightData),
        .type  = RHI::BufferType::Storage,
        .label = "LightsDataBuffer",
    });

    for (const auto& [i, light] : nbl::enumerate(initialLights))
    {
        if (i >= sMaxLights)
        {
            break;
        }
        mLights[i]   = light;
        mValidity[i] = true;
        mUploadQueue.push_back(i);
    }
    upload();
}

uint64_t LightSystem::addLight(const Light& light) noexcept
{
    // Find first index marked as "invalid"
    const auto it = std::ranges::find_if(mValidity, [](const bool b) -> bool { return b == false; });
    if (it == std::end(mValidity))
    {
        exitWithError("Failed to add new light: Max Light count reached (n={})", sMaxLights);
    }

    const auto index = std::distance(std::begin(mValidity), it);

    mValidity[index] = true;
    mLights[index]   = light;
    if (light.name == "Light")
    {
        mLights[index].name = std::format("Light #{}", index);
    }

    mUploadQueue.push_back(index);

    return static_cast<uint64_t>(index);
}

void LightSystem::upload() noexcept
{
    if (mUploadQueue.empty())
    {
        // Nothing to do.
        return;
    }
    constexpr auto elementSize = sizeof(GPULightData);

    // Prepare staging buffer
    const auto uploadSize    = mUploadQueue.size() * elementSize;
    const auto stagingBuffer = mRHI->createBuffer({
        uploadSize, RHI::BufferType::Staging, "LightSystem::upload()-Staging"
    });

    // Prepare data and copy regions
    std::vector<GPULightData>    data(mUploadQueue.size());
    std::vector<vk::BufferCopy2> regions;
    for (const auto& [uploadQueueIndex, lightIndex] : nbl::enumerate(mUploadQueue))
    {
        const auto region = vk::BufferCopy2()
                            .setSrcOffset(uploadQueueIndex * elementSize)
                            .setDstOffset(lightIndex * elementSize)
                            .setSize(elementSize);

        regions.push_back(region);
        data.push_back(GPULightData::fromLight(mLights[lightIndex]));
    }

    // Set staging data then copy to LightsBuffer
    stagingBuffer->setData(data.data(), uploadSize, 0);

    mRHI->getGraphicsQueue()->immediate([&](const RHI::CommandList* pCommandList) -> void {
        const auto copyInfo = vk::CopyBufferInfo2()
                              .setSrcBuffer(stagingBuffer->getHandle())
                              .setDstBuffer(mLightsBuffer->getHandle())
                              .setRegions(regions);
        pCommandList->getHandle().copyBuffer2(copyInfo);
    });

    spdlog::debug("[Light System] Uploaded {} light(s).", mUploadQueue.size());
    mUploadQueue.clear();
}

uint64_t LightSystem::getCount() const noexcept
{
    return std::ranges::count(mValidity, true);
}

const SPtr<RHI::Buffer>& LightSystem::getDataBuffer() const noexcept
{
    return mLightsBuffer;
}

std::set<uint64_t> LightSystem::getValidIndices() const noexcept
{
    std::set<uint64_t> indices;
    for (const auto& [i, val] : nbl::enumerate(mValidity))
    {
        if (val)
        {
            indices.insert(i);
        }
    }
    return indices;
}

std::vector<Light*> LightSystem::getValidLights() noexcept
{
    std::vector<Light*> lights;
    for (auto&& [i, light] : nbl::enumerate(mLights))
    {
        if (mValidity[i])
        {
            lights.push_back(&light);
        }
    }
    return lights;
}
