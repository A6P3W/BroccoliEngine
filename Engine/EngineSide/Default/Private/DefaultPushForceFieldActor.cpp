#include "DefaultPushForceFieldActor.h"

#include "ForceFieldComponent.h"

REGISTER_ACTOR(ADefaultPushForceFieldActor)

ADefaultPushForceFieldActor::ADefaultPushForceFieldActor() {
  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (!ForceField) {
    return;
  }

  ForceField->SetForceType(EForceFieldType::Point);
  ForceField->SetStrength(0.75f);
  ForceField->SetActive(true);
}
