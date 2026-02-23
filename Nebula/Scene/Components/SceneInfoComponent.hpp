#pragma once

#include "UserInterface/IComponent.hpp"

class SceneV2;
class SceneInfoComponent final : public IComponent
{
public:
    explicit SceneInfoComponent(SceneV2* pScene);

    ~SceneInfoComponent() override = default;

    void draw() override;

private:
    SceneV2* mScene;
    std::size_t mLightIndex = 0;
};