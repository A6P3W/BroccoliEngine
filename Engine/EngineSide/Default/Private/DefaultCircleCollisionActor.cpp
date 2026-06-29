#include "DefaultCircleCollisionActor.h"

#include <CircleCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultCircleCollisionActor);
ADefaultCircleCollisionActor::ADefaultCircleCollisionActor() {
  auto col = std::make_unique<MCircleCollisionComponent>();
  col->SetParentComponent(GetRootComponent());
  AddComponent(std::move(col));
}
