#pragma once
#include "Actor.h"
#include <Utils/Umath.h>

class MTransformComponent;

class ASampleA : public AActor
{
public:
    ASampleA(FVector2D location, FRotator rotation);
    ~ASampleA() override;

    void OnUpdate(float DeltaTime) override;

private:
};
