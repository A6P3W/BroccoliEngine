#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "SceneComponent.h"
#include "UMath.h"

class AActor;
class MCollisionComponent;

enum class EForceFieldType : uint8_t {
  Directional,
  Point,
};

class BROCCOLI_ENGINE_API MForceFieldComponent : public MSceneComponent {
 public:
  MForceFieldComponent();
  ~MForceFieldComponent() override = default;

  void OnUpdate(float DeltaTime) override;
  void SetRangeComponent(MCollisionComponent* NewRangeComponent);
  MCollisionComponent* GetRangeComponent() const;
  void SetForceType(EForceFieldType NewForceType);
  EForceFieldType GetForceType() const;
  void SetDirection(const FVector2D& NewDirection);
  const FVector2D& GetDirection() const;
  void SetStrength(float NewStrength);
  float GetStrength() const;
  void SetAffectedActorTags(const std::vector<std::string>& NewTags);
  const std::vector<std::string>& GetAffectedActorTags() const;
  void SetIgnoredActorTags(const std::vector<std::string>& NewTags);
  const std::vector<std::string>& GetIgnoredActorTags() const;
  void SetActive(bool bNewActive);
  bool IsActive() const;
  void Pulse(float StrengthScale = 1.0f);

 private:
  enum class EApplicationType { Force, Impulse };

  std::vector<AActor*> ResolveTargets() const;
  bool IsValidTarget(AActor* Target) const;
  FVector2D GetForceOrigin() const;
  FVector2D CalculateVector(AActor* Target, float StrengthScale) const;
  void ApplyToTargets(EApplicationType ApplicationType, float StrengthScale);

  EForceFieldType ForceType = EForceFieldType::Directional;
  FVector2D Direction = {1.0f, 0.0f};
  float Strength = 1.0f;
  bool bActive = false;
  MCollisionComponent* RangeComponent = nullptr;
  std::vector<std::string> AffectedActorTags;
  std::vector<std::string> IgnoredActorTags;
};
