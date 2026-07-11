#include "DefaultLineCollisionActor.h"

#include <LineCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultLineCollisionActor);
ADefaultLineCollisionActor::ADefaultLineCollisionActor() {
  auto* Col = NewObject<MLineCollisionComponent>(this);
  if (Col) {
    Col->SetParentComponent(GetRootComponent());
    Col->RegisterComponent();
  }
}
