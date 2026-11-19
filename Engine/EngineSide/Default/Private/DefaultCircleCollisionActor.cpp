#include "DefaultCircleCollisionActor.h"

#include <memory>

#include "CircleCollision2DComponent.h"

REGISTER_ACTOR(ADefaultCircleCollisionActor);
ADefaultCircleCollisionActor::ADefaultCircleCollisionActor() {
  auto* Col = NewObject<MCircleCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
