#include "DefaultLineCollisionActor.h"

#include <LineCollision2DComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultLineCollisionActor);
ADefaultLineCollisionActor::ADefaultLineCollisionActor() {
  auto* Col = NewObject<MLineCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
