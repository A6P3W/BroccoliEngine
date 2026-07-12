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

struct FAttachmentTransformRules {
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

inline constexpr FAttachmentTransformRules FAttachmentTransformRules::KeepRelativeTransform{
    EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative};
inline constexpr FAttachmentTransformRules FAttachmentTransformRules::KeepWorldTransform{
    EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld};
inline constexpr FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetIncludingScale{
    EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget};

class BROCCOLI_ENGINE_API MSceneComponent : public MActorComponent {
 public:
  MSceneComponent();
  virtual ~MSceneComponent();
  virtual void OnUpdate(float DeltaTime);
  virtual void Draw();

  virtual void OnMessage(const std::string& message);

  void AttachToComponent(MSceneComponent* Parent);
  bool AttachToComponent(MSceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules);
  MSceneComponent* GetParentComponent() const;

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
  bool IsVisible() const;

  bool bGridDirty() const;
  void SetGridClean();

 protected:
  void OnComponentDestroy() override;

  const std::vector<MSceneComponent*>& GetChildComponents() const;
  void MakeTransformDirty();
  void UpdateTransform() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
