#include "ForceFieldActor.h"

#include <cstdint>
#include <string>

#include "CircleCollisionComponent.h"
#include "DebugOverlay.h"
#include "ForceFieldComponent.h"

REGISTER_ACTOR(AForceFieldActor)

AForceFieldActor::AForceFieldActor() {
  bReplicates = true;
  ForceFieldComponent = NewObject<MForceFieldComponent>(this);
  if (!ForceFieldComponent) {
    return;
  }
  SetRootComponent(ForceFieldComponent);
  ForceFieldComponent->RegisterComponent();

  RangeComponent = NewObject<MCircleCollisionComponent>(this);
  if (!RangeComponent) {
    return;
  }
  RangeComponent->AttachToComponent(ForceFieldComponent);
  RangeComponent->SetRadius(100.0f);
  RangeComponent->SetCollisionType(ECollisionType::Overlap);
  RangeComponent->RegisterComponent();

  ForceFieldComponent->SetRangeComponent(RangeComponent);
  ForceFieldComponent->SetForceType(EForceFieldType::Directional);
  ForceFieldComponent->SetDirection({1.0f, 0.0f});
  ForceFieldComponent->SetStrength(1.0f);
  ForceFieldComponent->SetActive(false);
}

MForceFieldComponent* AForceFieldActor::GetForceFieldComponent() const {
  return ForceFieldComponent;
}

MCollisionComponent* AForceFieldActor::GetRangeComponent() const { return RangeComponent; }

void AForceFieldActor::OnUpdate(float DeltaTime) {
  AActor::OnUpdate(DeltaTime);

  const std::string LogKey =
      "ForceFieldStatus_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
  const char* State =
      ForceFieldComponent && ForceFieldComponent->IsActive() ? "ACTIVE" : "INACTIVE";
  const char* AuthorityState = bHasAuthority ? "Authority" : "Client";
  const char* Type =
      ForceFieldComponent && ForceFieldComponent->GetForceType() == EForceFieldType::Point
          ? "Point"
          : "Directional";
  const std::size_t OverlapCount =
      RangeComponent ? RangeComponent->GetOverlappingActors().size() : 0;

  DRAW_WORLD_LOG(
      LogKey,
      0.1f,
      this,
      "ForceField {} / {} / {} / Overlaps={} / Strength={:.2f}",
      State,
      AuthorityState,
      Type,
      OverlapCount,
      ForceFieldComponent ? ForceFieldComponent->GetStrength() : 0.0f
  );
}
