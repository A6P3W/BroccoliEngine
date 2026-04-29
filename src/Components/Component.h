#pragma once
#include <string>

class OUpdateableObject; // 前方宣言

class Component {
public:
    virtual ~Component();
    virtual void Update(float DeltaTime);
    virtual void Draw();

    // メッセージ受信用の仮想関数
    virtual void OnMessage(const std::string& message);

    void SetOwner(OUpdateableObject* owner);

protected:
    OUpdateableObject* m_owner = nullptr;
};