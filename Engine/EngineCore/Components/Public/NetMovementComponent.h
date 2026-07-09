#pragma once

#include <cstdint>
#include <vector>

#include "MovementComponent.h"
#include "NetworkTypes.h"
#include "UMath.h"

class MNetMovementComponent : public MMovementComponent {
 public:
  MNetMovementComponent();

  void RegisterComponent() override;
  void OnUpdate(float DeltaTime) override;

  void AddMovementInput(const FVector2D& Input);
  void AddWorldForce(const FVector2D& Force) override;
  void AddLocalForce(const FVector2D& Force) override;
  void SetWorldForce(const FVector2D& Force) override;
  void SetLocalForce(const FVector2D& Force) override;
  void AddVelocityRotation(const FRotator& Rotation) override;
  void SetVelocityRotation(const FRotator& Rotation) override;

  void SetMaxSpeed(float NewMaxSpeed) { MaxSpeed = NewMaxSpeed; }
  float GetMaxSpeed() const { return MaxSpeed; }
  void SetAcceleration(float NewAcceleration) { Acceleration = NewAcceleration; }
  float GetAcceleration() const { return Acceleration; }

 protected:
  virtual void SimulateMovement(const FMovePredictionData& Move);
  void ApplyMovementData(const FVector2D& DesiredOffset);

 private:
  void Server_UploadMove(FMovePredictionData Move);
  void Client_ResolveMove(
      uint32_t AckedSequence,
      FVector2D ServerLocation,
      FRotator ServerRotation,
      FVector2D ServerVelocity
  );
  bool ShouldSimulate() const;
  void ClearFrameMovementData();

  FVector2D CurrentForce = FVector2D::ZeroVector;
  FRotator VelocityRotation = FRotator(0.0f);
  FVector2D FrameVelocityOverride = FVector2D::ZeroVector;
  bool bFrameVelocityOverride = false;
  std::vector<FMovePredictionData> InFlightMoves;
  std::vector<FMovePredictionData> ServerPendingMoves;
  uint32_t CurrentSequence = 0;
  uint32_t LastConfirmedSequence = 0;
  float MaxSpeed = 320.0f;
  float Acceleration = 2800.0f;
};
