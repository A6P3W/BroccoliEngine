#pragma once
#include <cstdint>
#include <vector>

#include "SceneComponent.h"

class FShakeHandle {
 public:
  FShakeHandle() = default;
  explicit FShakeHandle(uint64_t InId) : m_Id(InId) {}

  bool IsValid() const { return m_Id != 0; }
  void Invalidate() { m_Id = 0; }
  uint64_t GetId() const { return m_Id; }
  bool operator==(const FShakeHandle& Other) const { return m_Id == Other.m_Id; }
  bool operator!=(const FShakeHandle& Other) const { return !(*this == Other); }

 private:
  uint64_t m_Id = 0;
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

  std::vector<FShakeInstance> m_activeShakes;
  uint64_t m_nextShakeId = 1;
  FVector2D m_lastAppliedLocalOffset = FVector2D::ZeroVector;
  FVector2D m_baseLocalLocation = FVector2D::ZeroVector;
  bool m_hasBaseLocalLocation = false;
};
