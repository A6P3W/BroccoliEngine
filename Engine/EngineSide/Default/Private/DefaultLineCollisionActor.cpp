#include "DefaultLineCollisionActor.h"

#include <LineCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultLineCollisionActor);
ADefaultLineCollisionActor::ADefaultLineCollisionActor() {
  auto* Col = NewObject<MLineCollisionComponent>(this);
  if (Col) {
    Col->AttachToComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
