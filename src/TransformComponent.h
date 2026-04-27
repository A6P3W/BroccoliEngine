#pragma once
#include "Component.h"
#include "GameObject.h"
class TransformComponent : public Component
{
public:
	float x = 0.0f, y = 0.0f;
	void SetPos(float nx, float ny) { x = nx;y = ny; }
  float GetX() const { return x; }
	float GetY() const { return y; }
};

