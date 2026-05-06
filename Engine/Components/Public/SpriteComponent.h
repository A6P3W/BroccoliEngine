#pragma once
#include "Components/Public/SceneComponent.h"

class MSpriteComponent : public MSceneComponent {
public:
    MSpriteComponent(int handle, int priority = 0);

    void Draw() override;

    void OnMessage(const std::string& message) override;

private:
    int m_handle;
    int m_priority = 0;
};
