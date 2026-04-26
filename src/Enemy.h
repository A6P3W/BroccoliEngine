#pragma once
#include "GameObject.h"

class Enemy : public GameObject
{
public:
    Enemy(float x, float y);
    ~Enemy() override;

    bool Update() override;
    bool Draw() override;
};