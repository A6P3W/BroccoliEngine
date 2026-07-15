#pragma once
#include "BroccoliEngineAPI.h"
#include "ActorComponent.h"
#include "UMath.h"
class BROCCOLI_ENGINE_API MMovementComponent : public MActorComponent {
 public:
  virtual void AddWorldForce(const FVector2D& Force);
  virtual void AddLocalForce(const FVector2D& Force);
  virtual void SetWorldForce(const FVector2D& Force);
  virtual void SetLocalForce(const FVector2D& Force);
  virtual void SetWorldVelocity(const FVector2D& NewVelocity);
  virtual void SetLocalVelocity(const FVector2D& NewVelocity);
  virtual void AddWorldVelocity(const FVector2D& DeltaVelocity);
  virtual void AddLocalVelocity(const FVector2D& DeltaVelocity);
  virtual void AddVelocityRotation(const FRotator& Rotation);
  virtual void SetVelocityRotation(const FRotator& Rotation);

  FVector2D GetVelocity() const { return Velocity; }
  float GetVelocitySizeSquared() const { return Velocity.SizeSquared(); }
  float GetFriction() const { return Friction; }
  void SetFriction(float NewFriction) { Friction = NewFriction; }
  void OnUpdate(float DeltaTime) override;

 protected:
  struct FMovementStepData {
    float DeltaTime = 0.0f;
    FVector2D Force = FVector2D::ZeroVector();
    FVector2D StartVelocity = FVector2D::ZeroVector();
    FRotator StartRotation = FRotator(0.0f);
    FRotator VelocityRotation = FRotator(0.0f);
  };

  virtual void SimulateMovementStep(const FMovementStepData& Step);
  virtual void ApplyMovementOffset(const FVector2D& Offset);
  void ClearPendingMovement();

  FVector2D Velocity = FVector2D::ZeroVector();
  FVector2D PendingForce = FVector2D::ZeroVector();
  FRotator PendingVelocityRotation = FRotator(0.0f);
  float Friction = 0.95f;
};
