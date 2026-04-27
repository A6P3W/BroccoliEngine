#pragma once
#include "GameObject.h"

class TransformComponent;

class Enemy : public GameObject
{
public:
    Enemy(float x, float y);
    ~Enemy() override;

    void OnUpdate() override;
    void OnDraw() override;

private:
    TransformComponent* m_transform = nullptr;
};