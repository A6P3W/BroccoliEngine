#pragma once
#include "OUpdateableObject.h"
#include <string>
#include "TransformComponent.h"
class TransformComponent;
class OGameObject :
    public OUpdateableObject
{
public:
    OGameObject();
    virtual void OnUpdate() override;
    virtual void Draw()final;
protected:
    virtual void OnDraw() {}
    TransformComponent* m_transform = nullptr;
private:
};

