#pragma once
#include "Component.h"

class SpriteComponent : public Component {
public:
    SpriteComponent(int handle);

    void Draw() override;

    void OnMessage(const std::string& message) override;

private:
    int m_handle;
};