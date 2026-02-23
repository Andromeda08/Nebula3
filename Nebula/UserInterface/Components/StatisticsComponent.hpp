#pragma once

#include "Core/Types.hpp"
#include "UserInterface/IComponent.hpp"

namespace RHI
{
    class VulkanRHI;
}

class StatisticsComponent final : public IComponent
{
public:
    explicit StatisticsComponent(const SPtr<RHI::VulkanRHI>& rhi);

    ~StatisticsComponent() override = default;

    void draw() override;

private:
    SPtr<RHI::VulkanRHI> mRHI;
};