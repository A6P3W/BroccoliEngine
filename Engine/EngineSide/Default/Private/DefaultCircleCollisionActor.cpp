#include "DefaultCircleCollisionActor.h"

#include <CircleCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultCircleCollisionActor);
ADefaultCircleCollisionActor::ADefaultCircleCollisionActor() {
  auto* Col = NewObject<MCircleCollisionComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
