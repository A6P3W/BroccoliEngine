#pragma once
#include <cstdint>

#include "BroccoliEngineAPI.h"
#include "MovementComponent.h"
#include "NetworkTypes.h"
#include "UMath.h"

class BROCCOLI_ENGINE_API MNetMovementComponent : public MMovementComponent {
 public:
  MNetMovementComponent();
  ~MNetMovementComponent() override;

  void OnRegister() override;
  void OnUpdate(float DeltaTime) override;

  void AddWorldForce(const FVector2D& Force) override;
  void AddLocalForce(const FVector2D& Force) override;
  void SetWorldForce(const FVector2D& Force) override;
  void SetLocalForce(const FVector2D& Force) override;
  void SetWorldVelocity(const FVector2D& NewVelocity) override;
  void SetLocalVelocity(const FVector2D& NewVelocity) override;
  void AddWorldImpulse(const FVector2D& Impulse) override;
  void AddLocalImpulse(const FVector2D& Impulse) override;
  void AddVelocityRotation(const FRotator& Rotation) override;
  void SetVelocityRotation(const FRotator& Rotation) override;

 protected:
  virtual void SimulateMovement(const FMovePredictionData& Move);

  void Server_UploadMove(FMovePredictionData Move);
  void Client_ResolveMove(
      uint32_t AckedSequence,
      FVector2D ServerLocation,
      FRotator ServerRotation,
      FVector2D ServerVelocity
  );
  bool ShouldSimulate() const;
  void ClearFrameMovementData();
  void Client_ReceiveAuthorityCorrection(
      uint32_t ConfirmedSequence,
      uint32_t CorrectionSequence,
      FVector2D ServerLocation,
      FRotator ServerRotation,
      FVector2D ServerVelocity
  );

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
