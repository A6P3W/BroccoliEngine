#include "UpdateableComponent.h"

bool UpdateableComponent::Update(float DeltaTime)
{
	this->OnUpdate(DeltaTime);
	return true;
}
