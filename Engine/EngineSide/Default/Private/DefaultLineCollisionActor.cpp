#include "DefaultLineCollisionActor.h"

#include <memory>

#include "LineCollision2DComponent.h"

REGISTER_ACTOR(ADefaultLineCollisionActor);
ADefaultLineCollisionActor::ADefaultLineCollisionActor() {
  auto* Col = NewObject<MLineCollision2DComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
