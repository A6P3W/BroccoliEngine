#pragma once
#include "CollisionComponent.h"

class MCircleCollisionComponent : public MCollisionComponent
{
public:
	MCircleCollisionComponent(float radius = 50.0f) { m_radius = radius; }
	ECollisionShape GetShapeType() const override { return ECollisionShape::Circle; }
	float GetRadius() const{
		return m_radius;
	};
	void Draw() override;

	FAABB GetAABB() const override;
private:
	float m_radius;
};
