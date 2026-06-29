#pragma once
#include "ActorComponent.h"
#include "UMath.h"
class MMovementComponent : public MActorComponent {
 public:
  void AddWorldForce(const FVector2D& Force);
  void AddLocalForce(const FVector2D& Force);
  void SetWorldForce(const FVector2D& Force);
  void SetLocalForce(const FVector2D& Force);

  void AddVelocityRotation(const FRotator& Rotation);
  void SetVelocityRotation(const FRotator& Rotation);

  FVector2D GetVelocity() const { return Velocity; }
  float GetVelocitySizeSquared() const { return Velocity.SizeSquared(); }
  void OnUpdate(float DeltaTime) override;

 private:
  FVector2D Velocity = {0.0f, 0.0f};
  float Friction = 0.95f;
};
