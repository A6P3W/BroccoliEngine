#pragma once
#include "BroccoliEngineAPI.h"
#include <cstdint>
 
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
 
 class BROCCOLI_ENGINE_API MEasyShakeComponent : public MSceneComponent {
  public:
   MEasyShakeComponent();
   ~MEasyShakeComponent() override;
 
   void ApplyShake(
       const FVector2D& StrengthXY, float DurationSeconds, bool bEnableFadeIn, bool bEnableFadeOut
   );
 
   void StartShake(FShakeHandle& Handle, const FVector2D& StrengthXY, float DurationSeconds);
   void EndShake(FShakeHandle& Handle, bool bEnableFadeOut = false);
   bool IsShakeActive(const FShakeHandle& Handle) const;
 
   void OnUpdate(float DeltaTime) override;
 
  private:
   struct Impl;
   Impl* ImplPtr = nullptr;
 };
