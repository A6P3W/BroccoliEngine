#include "DefaultDirectionalForceFieldActor.h"

#include "ForceFieldComponent.h"

REGISTER_ACTOR(ADefaultDirectionalForceFieldActor)

ADefaultDirectionalForceFieldActor::ADefaultDirectionalForceFieldActor() {
  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (!ForceField) {
    return;
  }

  ForceField->SetForceType(EForceFieldType::Directional);
  ForceField->SetDirection({1.0f, 0.0f});
  ForceField->SetStrength(0.75f);
  ForceField->SetActive(true);
}

void ADefaultDirectionalForceFieldActor::OnUpdate(float DeltaTime) {
  AForceFieldActor::OnUpdate(DeltaTime);

  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (ForceField) {
    ForceField->SetDirection(FVector2D{1.0f, 0.0f}.RotateVector(GetActorRotation()));
  }
}
