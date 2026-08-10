#pragma once

#include "Light.hpp"
#include "../SlotPool.hpp"

namespace nbl
{
    class LightSystem : public Pool<Light, GPULightData>
    {
    public:
        explicit LightSystem(const SPtr<RHI::VulkanRHI>& rhi)
        : Pool(rhi, "LightSystem", 1024)
        {
            for (const auto& type : getLightTypes())
            {
                mLightTypeNames.push_back(toString(type));
            }
        }

        void onUpdate(const RHI::CommandList* commandList)
        {
            flush(commandList);
        }

        [[nodiscard]] PoolHandle addLight(const Light& light)
        {
            const auto hLight = acquire(light);
            if (mSelectedLight.isNull())
            {
                mSelectedLight = hLight;
            }
            return hLight;
        }

        void onDrawUI();

    private:
        std::vector<std::string> mLightTypeNames = {};
        PoolHandle               mSelectedLight  = {};
    };
}
