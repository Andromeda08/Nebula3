#pragma once

class IComponent
{
public:
    virtual ~IComponent() = default;

    virtual void update() {};

    virtual void draw() = 0;
};
