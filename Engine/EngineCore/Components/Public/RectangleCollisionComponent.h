#pragma once
#include "CollisionComponent.h"

class MRectangleCollisionComponent : public MCollisionComponent
{
public:
	MRectangleCollisionComponent(float width = 100.0f, float height = 100.0f)
		: m_width(width), m_height(height) {}

	ECollisionShape GetShapeType() const override { return ECollisionShape::Rectangle; }

	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }

	void Draw() override;
	FAABB GetAABB() const override;

private:
	float m_width;
	float m_height;
};
