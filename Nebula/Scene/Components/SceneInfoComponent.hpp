#pragma once

#include "UserInterface/IComponent.hpp"

class Scene;
class SceneInfoComponent final : public IComponent
{
public:
    explicit SceneInfoComponent(Scene* pScene);

    ~SceneInfoComponent() override = default;

    void draw() override;

private:
    Scene* mScene;

    bool mUseSubsurfaceScattering;
    float mRadius;
    float mScale;
};