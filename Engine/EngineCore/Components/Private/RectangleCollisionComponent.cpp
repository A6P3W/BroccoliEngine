#include "RectangleCollisionComponent.h"
#include <RenderSystem.h>
#include "EngineDefine.h"

void MRectangleCollisionComponent::Draw()
{
	if (!IsDebug)return;
	FVector2D center = GetWorldLocation();
	float halfWidth = (m_width * GetScale()) * 0.5f;
	float halfHeight = (m_height * GetScale()) * 0.5f;

	RenderSystem::GetInstance().SubmitBox(
		center.X - halfWidth,
		center.Y - halfHeight,
		center.X + halfWidth,
		center.Y + halfHeight,
		GetWorldRotation().Rotation,
		0x00FF00,
		0,
		RenderSpace::World,
		100,
		120);
}

FAABB MRectangleCollisionComponent::GetAABB() const
{
	FVector2D center = GetWorldLocation();
	float halfWidth = (m_width * GetScale()) * 0.5f;
	float halfHeight = (m_height * GetScale()) * 0.5f;

	FAABB aabb;
	aabb.MinX = center.X - halfWidth;
	aabb.MinY = center.Y - halfHeight;
	aabb.MaxX = center.X + halfWidth;
	aabb.MaxY = center.Y + halfHeight;
	return aabb;
}
