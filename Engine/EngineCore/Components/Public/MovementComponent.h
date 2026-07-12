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
  virtual void AddVelocityRotation(const FRotator& Rotation);
  virtual void SetVelocityRotation(const FRotator& Rotation);

  FVector2D GetVelocity() const { return Velocity; }
  float GetVelocitySizeSquared() const { return Velocity.SizeSquared(); }
  float GetFriction() const { return Friction; }
  void SetFriction(float NewFriction) { Friction = NewFriction; }
  void OnUpdate(float DeltaTime) override;

 protected:
  FVector2D Velocity = {0.0f, 0.0f};
  float Friction = 0.95f;
};
