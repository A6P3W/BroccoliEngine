#pragma once
#include <string>
#include <vector>

#include "ActorComponent.h"
#include "BroccoliEngineAPI.h"
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
      EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule
  )
      : LocationRule(InLocationRule), RotationRule(InRotationRule), ScaleRule(InScaleRule) {}

  static const FAttachmentTransformRules KeepRelativeTransform;
  static const FAttachmentTransformRules KeepWorldTransform;
  static const FAttachmentTransformRules SnapToTargetIncludingScale;
};

inline constexpr FAttachmentTransformRules FAttachmentTransformRules::KeepRelativeTransform{
    EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative
};
inline constexpr FAttachmentTransformRules FAttachmentTransformRules::KeepWorldTransform{
    EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld
};
inline constexpr FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetIncludingScale{
    EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget
};

/// @brief 座標管理コンポーネント。
class BROCCOLI_ENGINE_API MSceneComponent : public MActorComponent {
 public:
  DEFINE_ACTOR_COMPONENT_CLASS(MSceneComponent)
  MSceneComponent();
  virtual ~MSceneComponent();
  virtual void OnUpdate(float DeltaTime);
  virtual void Draw();

  virtual void OnMessage(const std::string& message);

  void AttachToComponent(MSceneComponent* Parent);
  bool AttachToComponent(MSceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules);
  MSceneComponent* GetParentComponent() const;

  bool SetWorldLocation2D(const FVector2D& NewWorldLocation);
  bool SetRelativeLocation2D(const FVector2D& NewRelativeLocation);
  bool AddWorldOffset2D(const FVector2D& Offset);
  bool AddLocalOffset2D(const FVector2D& Offset);
  FVector2D GetWorldLocation2D() const;
  FVector2D GetRelativeLocation2D() const;

  bool SetWorldRotation2D(FRotator NewRotation);
  bool AddWorldRotation2D(FRotator DeltaRotation);
  FRotator GetWorldRotation2D() const;
  FRotator GetRelativeRotation2D() const;

  bool SetRelativeScale2D(FScale NewScale);
  FScale GetRelativeScale2D() const;

  bool SetWorldScale2D(FScale NewScale);
  FScale GetWorldScale2D() const;

  bool SetWorldLocation3D(const FVector3D& NewWorldLocation);
  bool SetRelativeLocation3D(const FVector3D& NewRelativeLocation);
  FVector3D GetWorldLocation3D() const;
  FVector3D GetRelativeLocation3D() const;
  bool SetWorldRotation3D(const FQuaternion& NewRotation);
  bool SetRelativeRotation3D(const FQuaternion& NewRotation);
  FQuaternion GetWorldRotation3D() const;
  FQuaternion GetRelativeRotation3D() const;
  bool SetWorldScale3D(const FScale3D& NewScale);
  bool SetRelativeScale3D(const FScale3D& NewScale);
  FScale3D GetWorldScale3D() const;
  FScale3D GetRelativeScale3D() const;
  FTransform3D GetWorldTransform3D() const;
  FTransform3D GetRelativeTransform3D() const;

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
