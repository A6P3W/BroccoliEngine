#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Component.h"

class GameObject {
public:
    GameObject(float x, float y) : m_x(x), m_y(y) {}
    virtual ~GameObject() = default;
    // コンポーネントの追加
    void AddComponent(std::unique_ptr<Component> comp) {
        comp->SetOwner(this);
        m_components.push_back(std::move(comp));
    }

    virtual bool Update() {
        for (auto& comp : m_components) comp->Update();
        return true;
    }

    virtual bool Draw() {
        for (auto& comp : m_components) comp->Draw();
        return true;
    }

    // メッセージング（全コンポーネントに通知）
    void SendMessage(const std::string& message) {
        for (auto& comp : m_components) comp->OnMessage(message);
    }

    // アクセサ
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    void SetPos(float x, float y) { m_x = x; m_y = y; }

private:
    float m_x, m_y;
    std::vector<std::unique_ptr<Component>> m_components;
};