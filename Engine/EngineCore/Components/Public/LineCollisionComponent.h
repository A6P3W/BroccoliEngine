#pragma once
#include "CollisionComponent.h"

class MLineCollisionComponent : public MCollisionComponent
{
public:
	MLineCollisionComponent(const FVector2D& localStart = FVector2D::ZeroVector,
		const FVector2D& localEnd = { 100.0f, 0.0f })
		: m_localStart(localStart), m_localEnd(localEnd) {}

	ECollisionShape GetShapeType() const override { return ECollisionShape::Line; }

	FVector2D GetLocalStart() const { return m_localStart; }
	FVector2D GetLocalEnd() const { return m_localEnd; }

	FVector2D GetWorldStart() const { return GetWorldLocation() + m_localStart.RotateVector(GetWorldRotation()) * GetScale(); }
	FVector2D GetWorldEnd() const { return GetWorldLocation() + m_localEnd.RotateVector(GetWorldRotation()) * GetScale(); }

	void Draw() override;

private:
	FVector2D m_localStart;
	FVector2D m_localEnd;
};
