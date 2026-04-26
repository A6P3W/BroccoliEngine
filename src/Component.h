#pragma once
#include <string>

class GameObject; // 前方宣言

class Component {
public:
    virtual ~Component() = default;
    virtual void Update() {}
    virtual void Draw() {}

    // メッセージ受信用の仮想関数
    virtual void OnMessage(const std::string& message) {}

    void SetOwner(GameObject* owner) { m_owner = owner; }

protected:
    GameObject* m_owner = nullptr;
};