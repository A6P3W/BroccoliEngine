#include "Components/Public/SceneComponent.h"
#include "Actor.h"

bool MActorComponent::Update(float DeltaTime)
{
	OnUpdate(DeltaTime);
	return true;
}
