#include "DefaultLineCollisionActor.h"

#include <LineCollisionComponent.h>

#include <memory>

REGISTER_ACTOR(ADefaultLineCollisionActor);
ADefaultLineCollisionActor::ADefaultLineCollisionActor() {
  auto col = std::make_unique<MLineCollisionComponent>();
  col->SetParentComponent(GetRootComponent());
  AddComponent(std::move(col));
}
