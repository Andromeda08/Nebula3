#pragma once

#include <SDL3/SDL.h>
#include "Light.hpp"
#include "../SlotPool.hpp"
#include "UserInterface/IComponent.hpp"

namespace nbl
{
    class LightSystem : public Pool<Light, GPULightData>
    {
    public:
        explicit LightSystem(const SPtr<RHI::VulkanRHI>& rhi)
        : Pool(rhi, "LightSystem", 1024)
        {
        }

        void onUpdate(const RHI::CommandList* commandList)
        {
            flush(commandList);
        }
    };

    class LightSystemUI : public IComponent
    {
    public:
        explicit LightSystemUI(LightSystem* pLightSystem);

        void draw() override;

    private:
        std::vector<std::string> mLightTypeNames;
        LightSystem*             mLightSystem;
        Handle                   mSelectedLight;
    };
}
