#pragma once
#include "Component.h"
#include "GameObject.h"
#include "DxLib.h"

class SpriteComponent : public Component {
public:
    SpriteComponent(int handle) : m_handle(handle) {}

    void Draw() override {
        // オーナー（GameObject）から座標を取得して描画する
        DrawGraph((int)m_owner->GetX(), (int)m_owner->GetY(), m_handle, TRUE);
    }

    void OnMessage(const std::string& message) override {
        if (message == "HIT") {
            // 当たった時に何か演出をするなどの処理
        }
    }

private:
    int m_handle;
};