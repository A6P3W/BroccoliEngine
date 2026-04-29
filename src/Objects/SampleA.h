#pragma once
#include "Core/OGameObject.h"

class TransformComponent;

class SampleA : public OGameObject
{
public:
    SampleA(float x, float y);
    ~SampleA() override;

    void OnUpdate(float DeltaTime) override;
    void OnDraw() override;

private:
};