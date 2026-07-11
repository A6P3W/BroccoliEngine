#include "DefaultRectangleCollisionActor.h"

#include <RectangleCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultRectangleCollisionActor);
ADefaultRectangleCollisionActor::ADefaultRectangleCollisionActor() {
  auto* Col = NewObject<MRectangleCollisionComponent>(this);
  if (Col) {
    Col->SetParentComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
