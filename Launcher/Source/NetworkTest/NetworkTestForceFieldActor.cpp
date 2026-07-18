#include "NetworkTestForceFieldActor.h"

#include <DxLib.h>

#include "CircleCollisionComponent.h"
#include "ForceFieldComponent.h"
#include "SpriteComponent.h"

REGISTER_ACTOR(ANetworkTestForceFieldActor)

ANetworkTestForceFieldActor::ANetworkTestForceFieldActor() {
  MForceFieldComponent* ForceField = GetForceFieldComponent();
  if (ForceField) {
    ForceField->SetForceType(EForceFieldType::Directional);
    ForceField->SetDirection({1.0f, 0.0f});
    ForceField->SetStrength(0.5f);
    ForceField->SetAffectedActorTags({"ForceFieldAffected"});
    ForceField->SetActive(true);
  }

  MCircleCollisionComponent* Range = dynamic_cast<MCircleCollisionComponent*>(GetRangeComponent());
  if (Range) {
    Range->SetRadius(160.0f);
  }

  FieldMarker = NewObject<MSpriteComponent>(this);
  if (FieldMarker) {
    FieldMarker->SetRenderSettings(8, RenderSpace::World);
    FieldMarker->SubmitCircle(18.0f, FColor{80, 190, 255}, true);
    FieldMarker->RegisterComponent();
  }
}
