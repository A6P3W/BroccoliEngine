#include "CircleCollisionComponent.h"
#include <RenderSystem.h>
#include "EngineDefine.h"
void MCircleCollisionComponent::Draw()
{
	if (!IsDebug)return;
	RenderSystem::GetInstance().SubmitCircle(
		GetWorldLocation(),
		Radius * GetWorldScale().Scale,
		0x00FF00,
		0,
		RenderSpace::World,
		100,
		255
	);
	RenderSystem::GetInstance().SubmitCircle(
		GetWorldLocation(),
		Radius * GetWorldScale().Scale,
		11111111,
		1,
		RenderSpace::World,
		100,
		50
	);
}

FAABB MCircleCollisionComponent::GetAABB() const
{
	FVector2D center = GetWorldLocation();
	float radius = Radius * GetWorldScale().Scale;

	FAABB aabb;
	aabb.MinX = center.X - radius;
	aabb.MinY = center.Y - radius;
	aabb.MaxX = center.X + radius;
	aabb.MaxY = center.Y + radius;
	return aabb;
}
