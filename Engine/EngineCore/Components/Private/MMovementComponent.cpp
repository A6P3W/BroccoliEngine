#include "MMovementComponent.h"
#include <SceneComponent.h>
#include <Actor.h>
#include <Utils/UMath.h>
void MMovementComponent::AddForce(const FVector2D& Force)
{
}

void MMovementComponent::OnUpdate(float DeltaTime) 
{
	if (!GetOwner())return;
	AActor* owner = GetOwner();
	Velocity *= 0.96;
	owner->AddActorWorldOffset(Velocity);
}
