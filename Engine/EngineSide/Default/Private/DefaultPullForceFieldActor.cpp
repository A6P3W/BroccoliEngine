#include "DefaultPullForceFieldActor.h"

#include "ForceFieldComponent.h"

REGISTER_ACTOR(ADefaultPullForceFieldActor)

ADefaultPullForceFieldActor::ADefaultPullForceFieldActor() {
  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (!ForceField) {
    return;
  }

  ForceField->SetForceType(EForceFieldType::Point);
  ForceField->SetStrength(-0.75f);
  ForceField->SetActive(true);
}
