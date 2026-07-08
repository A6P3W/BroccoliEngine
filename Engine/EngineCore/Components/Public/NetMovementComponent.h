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
  void AddMovementAction(FMoveActionState Action);
  void SetMovementActions(FMoveActionState Actions);

  void SetMaxSpeed(float NewMaxSpeed) { MaxSpeed = NewMaxSpeed; }
  float GetMaxSpeed() const { return MaxSpeed; }
  void SetAcceleration(float NewAcceleration) { Acceleration = NewAcceleration; }
  float GetAcceleration() const { return Acceleration; }
  virtual float GetMaxSpeedForActions(FMoveActionState Actions) const;

 protected:
  virtual void SimulateMovement(const FMovePredictionData& Move);
  void ApplyMovementData(const FVector2D& DesiredOffset);

 private:
  void Server_UploadMove(FMovePredictionData Move);
  void Client_ResolveMove(
      uint32_t AckedSequence, FVector2D ServerLocation, FVector2D ServerVelocity
  );
  bool ShouldSimulate() const;

  FVector2D CurrentInputAxis = FVector2D::ZeroVector;
  FMoveActionState CurrentActions = EMoveAction::None;

  std::vector<FMovePredictionData> InFlightMoves;
  std::vector<FMovePredictionData> ServerPendingMoves;
  uint32_t CurrentSequence = 0;
  uint32_t LastConfirmedSequence = 0;
  float MaxSpeed = 320.0f;
  float Acceleration = 2800.0f;
};
