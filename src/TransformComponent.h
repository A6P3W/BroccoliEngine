#pragma once
#include "Component.h"

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;
};

class TransformComponent : public Component
{
public:
	void SetPos(float nx, float ny);
	void SetPos(const Vector2& nPos);
	void AddLocalPos(float nx, float ny);
	void AddWorldPos(float nx, float ny);
	const Vector2& GetPos() const;

	void SetAngle(float nAngle);
	void AddAngle(float nAngleDeg);
	float GetAngle() const;

	void SetScale(float nScale);
	float GetScale() const;
private:
	Vector2 pos;
	float angle = 0.0f;
	float scale = 1.0f;
};

