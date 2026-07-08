#pragma once

#include <cstdint>
#include <vector>

#include "MovementComponent.h"
#include "NetworkTypes.h"
#include "UMath.h"

class MCharacterMovementComponent : public MMovementComponent {
 public:
  MCharacterMovementComponent();

  void RegisterComponent() override;
  void OnUpdate(float DeltaTime) override;

  void AddInputVector(const FVector2D& Input);
  void AddInputFlag(uint8_t Flag);

  void SetMaxSpeed(float NewMaxSpeed) { MaxSpeed = NewMaxSpeed; }
  float GetMaxSpeed() const { return MaxSpeed; }
  void SetAcceleration(float NewAcceleration) { Acceleration = NewAcceleration; }
  float GetAcceleration() const { return Acceleration; }
  void SetDashSpeedMultiplier(float NewDashSpeedMultiplier) {
    DashSpeedMultiplier = NewDashSpeedMultiplier;
  }
  float GetDashSpeedMultiplier() const { return DashSpeedMultiplier; }

  virtual void PerformMovement(const FSavedMove& Move);
  void MoveWithCollision(const FVector2D& DesiredOffset);

 private:
  void Server_ReceiveMove(FSavedMove Move);
  void Client_ReceiveAck(
      uint32_t AckedSequence, FVector2D ServerLocation, FVector2D ServerVelocity
  );
  bool HasPendingSimulation() const;

  FVector2D CurrentInputAxis = FVector2D::ZeroVector;
  uint8_t CurrentInputFlags = 0;

  std::vector<FSavedMove> SavedMoves;
  std::vector<FSavedMove> ServerPendingMoves;
  uint32_t CurrentSequence = 0;
  uint32_t LastAckedSequence = 0;
  float MaxSpeed = 320.0f;
  float Acceleration = 2800.0f;
  float DashSpeedMultiplier = 1.5f;
};
