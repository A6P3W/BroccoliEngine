#include "LineCollisionComponent.h"
#include <RenderSystem.h>

void MLineCollisionComponent::Draw()
{
	FVector2D start = GetWorldStart();
	FVector2D end = GetWorldEnd();

	RenderSystem::GetInstance().SubmitLine(
		start.X,
		start.Y,
		end.X,
		end.Y,
		0x00FF00,
		RenderSpace::World,
		100,
		120);
}
