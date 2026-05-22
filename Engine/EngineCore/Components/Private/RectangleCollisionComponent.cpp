#include "RectangleCollisionComponent.h"
#include <RenderSystem.h>

void MRectangleCollisionComponent::Draw()
{
	FVector2D center = GetWorldLocation();
	float halfWidth = (m_width * GetScale()) * 0.5f;
	float halfHeight = (m_height * GetScale()) * 0.5f;

	RenderSystem::GetInstance().SubmitBox(
		center.X - halfWidth,
		center.Y - halfHeight,
		center.X + halfWidth,
		center.Y + halfHeight,
		0x00FF00,
		0,
		RenderSpace::World,
		100,
		120);
}
