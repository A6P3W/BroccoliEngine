#include "DefaultRectangleCollisionActor.h"

#include <memory>

#include "RectangleCollision2DComponent.h"

REGISTER_ACTOR(ADefaultRectangleCollisionActor);
ADefaultRectangleCollisionActor::ADefaultRectangleCollisionActor() {
  auto* Col = NewObject<MRectangleCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
