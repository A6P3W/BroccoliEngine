#pragma once
#include "Component.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "DxLib.h"

class SpriteComponent : public Component {
public:
    SpriteComponent(int handle) : m_handle(handle) {}

    void Draw() override {
        auto transform = m_owner ? m_owner->GetComponent<TransformComponent>() : nullptr;
        if (!transform) {
            return;
        }

        DrawGraph((int)transform->GetX(), (int)transform->GetY(), m_handle, TRUE);
    }

    void OnMessage(const std::string& message) override {
        if (message == "HIT") {
            // 当たった時に何か演出をするなどの処理
        }
    }

private:
    int m_handle;
};