#pragma once

#include "UserInterface/IComponent.hpp"

class Scene;
class SceneInfoComponent final : public IComponent
{
public:
    explicit SceneInfoComponent(const Scene* pScene);

    ~SceneInfoComponent() override = default;

    void draw() override;

private:
    const Scene* mScene;
};