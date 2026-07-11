#include "DefaultRectangleCollisionActor.h"

#include <RectangleCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultRectangleCollisionActor);
ADefaultRectangleCollisionActor::ADefaultRectangleCollisionActor() {
  auto* Col = NewObject<MRectangleCollisionComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
