#pragma once
#include "Core/GameObject.h"
#include <Utils/Umath.h>

class MTransformComponent;

class ASampleA : public AGameObject
{
public:
    ASampleA(FVector2D location, FRotator rotation);
    ~ASampleA() override;

    void OnUpdate(float DeltaTime) override;
    void OnDraw() override;

private:
};