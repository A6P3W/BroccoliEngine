#include "DefaultCircleCollisionActor.h"

#include <CircleCollision2DComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultCircleCollisionActor);
ADefaultCircleCollisionActor::ADefaultCircleCollisionActor() {
  auto* Col = NewObject<MCircleCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
