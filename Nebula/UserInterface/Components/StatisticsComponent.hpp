#pragma once

#include "UserInterface/IComponent.hpp"

class StatisticsComponent final : public IComponent
{
public:
    StatisticsComponent() = default;
    ~StatisticsComponent() override = default;

    void draw() override;
};