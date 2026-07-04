#pragma once
#include <cstdint>
#include <vector>

#include "SceneComponent.h"

class FShakeHandle {
 public:
  FShakeHandle() = default;
  explicit FShakeHandle(uint64_t InId) : ShakeID(InId) {}

  bool IsValid() const { return ShakeID != 0; }
  void Invalidate() { ShakeID = 0; }
  uint64_t GetId() const { return ShakeID; }
  bool operator==(const FShakeHandle& Other) const { return ShakeID == Other.ShakeID; }
  bool operator!=(const FShakeHandle& Other) const { return !(*this == Other); }

 private:
  uint64_t ShakeID = 0;
};

class MEasyShakeComponent : public MSceneComponent {
 public:
  void ApplyShake(
      const FVector2D& StrengthXY, float DurationSeconds, bool bEnableFadeIn, bool bEnableFadeOut
  );

  void StartShake(FShakeHandle& Handle, const FVector2D& StrengthXY, float DurationSeconds);
  void EndShake(FShakeHandle& Handle, bool bEnableFadeOut = false);
  bool IsShakeActive(const FShakeHandle& Handle) const;

  void OnUpdate(float DeltaTime) override;

 private:
  struct FShakeInstance {
    uint64_t Id = 0;
    FVector2D StrengthXY = FVector2D::ZeroVector;
    float DurationSeconds = 0.0f;
    float ElapsedSeconds = 0.0f;
    bool bFadeIn = false;
    bool bFadeOut = false;
  };

  std::vector<FShakeInstance> ActiveShakes;
  uint64_t NextShakeId = 1;
  FVector2D LastAppliedLocalOffset = FVector2D::ZeroVector;
  FVector2D BaseLocalLocation = FVector2D::ZeroVector;
  bool HasBaseLocalLocation = false;
};
