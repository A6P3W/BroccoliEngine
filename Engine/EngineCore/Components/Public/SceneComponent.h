#pragma once
#include "BroccoliEngineAPI.h"
#include <string>
#include <vector>

#include "ActorComponent.h"
#include "UMath.h"
class AActor;

enum class EAttachmentRule {
  KeepRelative,
  KeepWorld,
  SnapToTarget,
};

struct BROCCOLI_ENGINE_API FAttachmentTransformRules {
  EAttachmentRule LocationRule;
  EAttachmentRule RotationRule;
  EAttachmentRule ScaleRule;

  constexpr FAttachmentTransformRules(
      EAttachmentRule InLocationRule,
      EAttachmentRule InRotationRule,
      EAttachmentRule InScaleRule
  )
      : LocationRule(InLocationRule), RotationRule(InRotationRule), ScaleRule(InScaleRule) {}

  static const FAttachmentTransformRules KeepRelativeTransform;
  static const FAttachmentTransformRules KeepWorldTransform;
  static const FAttachmentTransformRules SnapToTargetIncludingScale;
};

class BROCCOLI_ENGINE_API MSceneComponent : public MActorComponent {
 public:
  MSceneComponent();
  virtual ~MSceneComponent();
  virtual void OnUpdate(float DeltaTime);
  virtual void Draw();

  virtual void OnMessage(const std::string& message);

  void AttachToComponent(MSceneComponent* Parent);
  bool AttachToComponent(MSceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules);
  auto GetParentComponent() const { return ParentComponent; }

  bool SetWorldLocation(const FVector2D& NewWorldLocation);
  bool SetRelativeLocation(const FVector2D& NewRelativeLocation);
  bool AddWorldOffset(const FVector2D& Offset);
  bool AddLocalOffset(const FVector2D& Offset);
  FVector2D GetWorldLocation() const;
  FVector2D GetRelativeLocation() const;

  bool SetWorldRotation(FRotator NewRotation);
  bool AddWorldRotation(FRotator DeltaRotation);
  FRotator GetWorldRotation() const;
  FRotator GetRelativeRotation() const;

  bool SetRelativeScale(FScale NewScale);
  FScale GetRelativeScale() const;

  bool SetWorldScale(FScale NewScale);
  FScale GetWorldScale() const;

  virtual void SetVisibility(bool bNewVisibility);
  bool IsVisible() const { return bVisible; }
  bool bVisible = true;

  bool bGridDirty() { return bIsGridDirty; }
  void SetGridClean() { bIsGridDirty = true; }

 protected:
  void OnComponentDestroy() override;

  MSceneComponent* ParentComponent = nullptr;
  std::vector<MSceneComponent*> ChildComponents;

  FVector2D RelativeLocation;
  FRotator RelativeRotation;
  FScale RelativeScale;

  mutable FVector2D WorldLocation;
  mutable FRotator WorldRotation;
  mutable FScale WorldScale;

  void MakeTransformDirty();
  void UpdateTransform() const;

  mutable bool bIsTransformDirty = true;

  mutable bool bIsGridDirty = true;
};
