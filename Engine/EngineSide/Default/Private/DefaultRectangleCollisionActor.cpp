#include "DefaultRectangleCollisionActor.h"

#include <RectangleCollisionComponent.h>
#include <memory>

REGISTER_ACTOR(ADefaultRectangleCollisionActor);
ADefaultRectangleCollisionActor::ADefaultRectangleCollisionActor()
{
	auto col = std::make_unique<MRectangleCollisionComponent>();
	col->SetParentComponent(GetRootComponent());
	AddComponent(std::move(col));
}
