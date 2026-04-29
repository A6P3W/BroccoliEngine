#pragma once
#include "OUpdateableObject.h"
#include <string>
#include "Components/TransformComponent.h"
class TransformComponent;
class OGameObject :
    public OUpdateableObject
{
public:
    OGameObject();
    virtual void OnUpdate(float DeltaTime) override;
    virtual void Draw()final;
    TransformComponent* GetTransform() const;
protected:
    virtual void OnDraw() {}
    TransformComponent* m_transform = nullptr;
private:
};

