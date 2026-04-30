#pragma once
#include "Core/GameObject.h"

class MTransformComponent;

class ASampleA : public AGameObject
{
public:
    ASampleA(float x, float y);
    ~ASampleA() override;

    void OnUpdate(float DeltaTime) override;
    void OnDraw() override;

private:
};