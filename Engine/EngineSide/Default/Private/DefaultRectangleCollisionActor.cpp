#include "DefaultRectangleCollisionActor.h"

#include <RectangleCollision2DComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultRectangleCollisionActor);
ADefaultRectangleCollisionActor::ADefaultRectangleCollisionActor() {
  auto* Col = NewObject<MRectangleCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
