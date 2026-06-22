#pragma once
#include "CollisionComponent.h"

class MCircleCollisionComponent : public MCollisionComponent
{
public:
	MCircleCollisionComponent(float radius = 50.0f) { Radius = radius; }
	ECollisionShape GetShapeType() const override { return ECollisionShape::Circle; }
	float GetRadius() const{
		return Radius;
	};
	void Draw() override;

	FAABB GetAABB() const override;
private:
	float Radius;
};
