#pragma once
#include <string>
#include "UpdateableComponent.h"
class MUpdateableObject; // 前方宣言

class MSceneComponent: public UpdateableComponent {
public:
    MSceneComponent();
    virtual ~MSceneComponent();
    virtual void OnUpdate(float DeltaTime);
    virtual void Draw();

    // メッセージ受信用の仮想関数
    virtual void OnMessage(const std::string& message);

    void SetOwner(MUpdateableObject* owner);

protected:
    MUpdateableObject* m_owner = nullptr;
};