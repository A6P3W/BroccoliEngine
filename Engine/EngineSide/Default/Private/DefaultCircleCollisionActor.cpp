#include "DefaultCircleCollisionActor.h"

#include <CircleCollisionComponent.h>
#include <memory>

ADefaultCircleCollisionActor::ADefaultCircleCollisionActor()
{
	auto col = std::make_unique<MCircleCollisionComponent>();
	col->SetParentComponent(GetRootComponent());
	AddComponent(std::move(col));
}

