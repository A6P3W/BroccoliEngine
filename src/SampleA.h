#pragma once
#include "OGameObject.h"

class TransformComponent;

class SampleA : public OGameObject
{
public:
    SampleA(float x, float y);
    ~SampleA() override;

    void OnUpdate() override;
    void OnDraw() override;

private:
};