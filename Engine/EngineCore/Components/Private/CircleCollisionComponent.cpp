#include "CircleCollisionComponent.h"
#include <RenderSystem.h>
#include "EngineDefine.h"
void MCircleCollisionComponent::Draw()
{
	if (!IsDebug)return;
	RenderSystem::GetInstance().SubmitCircle(
		GetWorldLocation(),
		m_radius * GetWorldScale(),
		0x00FF00,
		0,
		RenderSpace::World,
		100,
		120
	);
}

FAABB MCircleCollisionComponent::GetAABB() const
{
	FVector2D center = GetWorldLocation();
	float radius = m_radius * GetWorldScale();

	FAABB aabb;
	aabb.MinX = center.X - radius;
	aabb.MinY = center.Y - radius;
	aabb.MaxX = center.X + radius;
	aabb.MaxY = center.Y + radius;
	return aabb;
}
