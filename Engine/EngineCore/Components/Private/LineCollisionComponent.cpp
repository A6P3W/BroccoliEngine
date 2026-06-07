#include "LineCollisionComponent.h"
#include <RenderSystem.h>
#include "EngineDefine.h"

void MLineCollisionComponent::Draw()
{
	if (!IsDebug)return;

	RenderSystem::GetInstance().SubmitLine(
		GetWorldStart(),
		GetWorldEnd(),
		0x00FF00,
		RenderSpace::World,
		100,
		120);
}

FAABB MLineCollisionComponent::GetAABB() const
{
	FVector2D start = GetWorldStart();
	FVector2D end = GetWorldEnd();

	FAABB aabb;
	aabb.MinX = (start.X < end.X) ? start.X : end.X;
	aabb.MinY = (start.Y < end.Y) ? start.Y : end.Y;
	aabb.MaxX = (start.X > end.X) ? start.X : end.X;
	aabb.MaxY = (start.Y > end.Y) ? start.Y : end.Y;
	return aabb;
}
