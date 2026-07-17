#include "ForceFieldComponent.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "Actor.h"
#include "CollisionComponent.h"
#include "DebugOverlay.h"
#include "MovementComponent.h"
#if !defined(_RELEASE)
#include "ResourceManager.h"
#include "SpriteComponent.h"
#endif

MForceFieldComponent::MForceFieldComponent() {
  bReplicates = true;
  SetNetComponentName("ForceFieldComponent");
  RegisterReplicatedProperty(&ForceType);
  RegisterReplicatedProperty(&Direction);
  RegisterReplicatedProperty(&Strength);
  RegisterReplicatedProperty(&bActive);
}

#if !defined(_RELEASE)
void MForceFieldComponent::OnRegister() {
  if (DebugSprite || !GetOwner()) {
    return;
  }

  DebugSprite = NewObject<MSpriteComponent>(GetOwner());
  if (!DebugSprite) {
    return;
  }

  DebugSprite->SetRenderSettings(100, RenderSpace::World);
  UpdateDebugSprite();
  DebugSprite->RegisterComponent();
}
#endif

void MForceFieldComponent::OnUpdate(float DeltaTime) {
  MSceneComponent::OnUpdate(DeltaTime);
#if !defined(_RELEASE)
  UpdateDebugSprite();
#endif
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->bHasAuthority || !bActive) {
    return;
  }
  ApplyToTargets(EApplicationType::Force, 1.0f);
}

void MForceFieldComponent::SetRangeComponent(MCollisionComponent* NewRangeComponent) {
  if (NewRangeComponent && NewRangeComponent->GetOwner() != GetOwner()) {
    return;
  }
  RangeComponent = NewRangeComponent;
  if (RangeComponent) {
    RangeComponent->SetCollisionType(ECollisionType::Overlap);
  }
#if !defined(_RELEASE)
  UpdateDebugSprite();
#endif
}

MCollisionComponent* MForceFieldComponent::GetRangeComponent() const { return RangeComponent; }
void MForceFieldComponent::SetForceType(EForceFieldType NewForceType) {
  ForceType = NewForceType;
#if !defined(_RELEASE)
  UpdateDebugSprite();
#endif
}
EForceFieldType MForceFieldComponent::GetForceType() const { return ForceType; }
void MForceFieldComponent::SetDirection(const FVector2D& NewDirection) {
  Direction = NewDirection;
#if !defined(_RELEASE)
  UpdateDebugSprite();
#endif
}
const FVector2D& MForceFieldComponent::GetDirection() const { return Direction; }
void MForceFieldComponent::SetStrength(float NewStrength) {
  Strength = NewStrength;
#if !defined(_RELEASE)
  UpdateDebugSprite();
#endif
}
float MForceFieldComponent::GetStrength() const { return Strength; }
void MForceFieldComponent::SetAffectedActorTags(const std::vector<std::string>& NewTags) {
  AffectedActorTags = NewTags;
}

const std::vector<std::string>& MForceFieldComponent::GetAffectedActorTags() const {
  return AffectedActorTags;
}

void MForceFieldComponent::SetIgnoredActorTags(const std::vector<std::string>& NewTags) {
  IgnoredActorTags = NewTags;
}

const std::vector<std::string>& MForceFieldComponent::GetIgnoredActorTags() const {
  return IgnoredActorTags;
}

void MForceFieldComponent::SetActive(bool bNewActive) { bActive = bNewActive; }
bool MForceFieldComponent::IsActive() const { return bActive; }

void MForceFieldComponent::Pulse(float StrengthScale) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || !OwnerActor->bHasAuthority) {
    return;
  }
  ApplyToTargets(EApplicationType::Impulse, StrengthScale);
}

std::vector<AActor*> MForceFieldComponent::ResolveTargets() const {
  return RangeComponent ? RangeComponent->GetOverlappingActors() : std::vector<AActor*>{};
}

bool MForceFieldComponent::IsValidTarget(AActor* Target) const {
  AActor* OwnerActor = GetOwner();
  if (!Target || !OwnerActor || Target == OwnerActor || Target->IsPendingDestroy() ||
      Target->GetWorld() != OwnerActor->GetWorld()) {
    return false;
  }

  const bool Ignored = std::any_of(
      IgnoredActorTags.begin(), IgnoredActorTags.end(), [Target](const std::string& Tag) {
        return Target->HasTag(Tag);
      }
  );
  if (Ignored) {
    return false;
  }

  return AffectedActorTags.empty() ||
         std::any_of(
             AffectedActorTags.begin(), AffectedActorTags.end(), [Target](const std::string& Tag) {
               return Target->HasTag(Tag);
             }
         );
}

FVector2D MForceFieldComponent::GetForceOrigin() const {
  return RangeComponent ? RangeComponent->GetWorldLocation() : GetWorldLocation();
}

FVector2D MForceFieldComponent::CalculateVector(AActor* Target, float StrengthScale) const {
  if (!Target) {
    return FVector2D::ZeroVector();
  }
  FVector2D ForceDirection = ForceType == EForceFieldType::Point
                                 ? Target->GetActorLocation() - GetForceOrigin()
                                 : Direction;
  const float DirectionSizeSquared = ForceDirection.SizeSquared();
  if (DirectionSizeSquared <= 0.0001f) {
    return FVector2D::ZeroVector();
  }
  return ForceDirection / std::sqrt(DirectionSizeSquared) * Strength * StrengthScale;
}

#if !defined(_RELEASE)
void MForceFieldComponent::UpdateDebugSprite() {
  if (!DebugSprite) {
    return;
  }

  constexpr const char* DirectionalArrowPath = "Resources/.Engine/arrow-up.png";
  constexpr const char* PointPushArrowPath = "Resources/.Engine/arrow-up-from-dot.png";
  constexpr const char* PointPullArrowPath = "Resources/.Engine/arrow-down-to-dot.png";

  const char* ArrowPath = DirectionalArrowPath;
  if (ForceType == EForceFieldType::Point) {
    ArrowPath = Strength < 0.0f ? PointPullArrowPath : PointPushArrowPath;
  }

  DebugSprite->SubmitGraph(ResourceManager::GetInstance().LoadResourceGraph(ArrowPath));
  DebugSprite->SetWorldLocation(GetForceOrigin());

  if (ForceType == EForceFieldType::Directional && Direction.SizeSquared() > 0.0001f) {
    DebugSprite->SetWorldRotation(FRotator(UMath::RadToDeg(std::atan2(Direction.X, -Direction.Y))));
  } else {
    DebugSprite->SetWorldRotation(FRotator(0.0f));
  }
}
#endif

void MForceFieldComponent::ApplyToTargets(EApplicationType ApplicationType, float StrengthScale) {
  for (AActor* Target : ResolveTargets()) {
    if (!IsValidTarget(Target)) {
      continue;
    }
    std::vector<MMovementComponent*> MovementComponents =
        Target->GetComponents<MMovementComponent>();
    if (MovementComponents.empty() || !MovementComponents.front()) {
      continue;
    }
    const FVector2D Value = CalculateVector(Target, StrengthScale);
    if (Value.SizeSquared() <= 0.0001f) {
      continue;
    }
    const std::string LogKey = "ForceFieldApply_" +
                               std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "_" +
                               std::to_string(reinterpret_cast<std::uintptr_t>(Target));
    DRAW_WORLD_LOG(
        LogKey,
        0.1f,
        Target,
        "ForceField {} ({:.2f}, {:.2f})",
        ApplicationType == EApplicationType::Force ? "Force" : "Impulse",
        Value.X,
        Value.Y
    );

    if (ApplicationType == EApplicationType::Force) {
      MovementComponents.front()->AddWorldForce(Value);
    } else {
      MovementComponents.front()->AddWorldImpulse(Value);
    }
  }
}
