#pragma once
#include "Components/Component.h"

class SpriteComponent : public Component {
public:
    SpriteComponent(int handle, int priority = 0);

    void Draw() override;

    void OnMessage(const std::string& message) override;

private:
    int m_handle;
    int m_priority = 0;
};