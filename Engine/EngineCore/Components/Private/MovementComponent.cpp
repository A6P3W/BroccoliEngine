#include "MovementComponent.h"

#include <Actor.h>
#include <SceneComponent.h>

#include "EngineDefine.h"
#include "Log.h"
#include "UMath.h"

void MMovementComponent::AddWorldForce(const FVector2D& Force) {
  PendingForce = PendingForce + Force;
}

void MMovementComponent::AddLocalForce(const FVector2D& Force) {
  if (!GetOwner()) {
    AddWorldForce(Force);
    return;
  }
  AddWorldForce(Force.RotateVector(GetOwner()->GetActorRotation()));
}

void MMovementComponent::SetWorldForce(const FVector2D& Force) { PendingForce = Force; }

void MMovementComponent::SetLocalForce(const FVector2D& Force) {
  if (!GetOwner()) {
    SetWorldForce(Force);
    return;
  }
  SetWorldForce(Force.RotateVector(GetOwner()->GetActorRotation()));
}

void MMovementComponent::SetWorldVelocity(const FVector2D& NewVelocity) {
  Velocity = NewVelocity;
}

void MMovementComponent::SetLocalVelocity(const FVector2D& NewVelocity) {
  if (!GetOwner()) {
    SetWorldVelocity(NewVelocity);
    return;
  }
  SetWorldVelocity(NewVelocity.RotateVector(GetOwner()->GetActorRotation()));
}

void MMovementComponent::AddWorldVelocity(const FVector2D& DeltaVelocity) {
  Velocity = Velocity + DeltaVelocity;
}

void MMovementComponent::AddLocalVelocity(const FVector2D& DeltaVelocity) {
  if (!GetOwner()) {
    AddWorldVelocity(DeltaVelocity);
    return;
  }
  AddWorldVelocity(DeltaVelocity.RotateVector(GetOwner()->GetActorRotation()));
}

void MMovementComponent::AddVelocityRotation(const FRotator& Rotation) {
  PendingVelocityRotation = PendingVelocityRotation + Rotation;
}

void MMovementComponent::SetVelocityRotation(const FRotator& Rotation) {
  if (!GetOwner()) return;
  PendingVelocityRotation = Rotation - GetOwner()->GetActorRotation();
}

void MMovementComponent::OnUpdate(float DeltaTime) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || DeltaTime <= 0.0f) {
    ClearPendingMovement();
    return;
  }

  FMovementStepData Step;
  Step.DeltaTime = DeltaTime;
  Step.Force = PendingForce;
  Step.StartVelocity = Velocity;
  Step.StartRotation = OwnerActor->GetActorRotation();
  Step.VelocityRotation = PendingVelocityRotation;
  SimulateMovementStep(Step);
  ClearPendingMovement();
}

void MMovementComponent::SimulateMovementStep(const FMovementStepData& Step) {
  if (Step.DeltaTime <= 0.0f) return;

  if (AActor* OwnerActor = GetOwner()) {
    OwnerActor->SetActorRotation(Step.StartRotation);
  }

  const float FrameScale = Step.DeltaTime * ReferenceFrameRate;
  Velocity = Step.StartVelocity;
  Velocity += Step.Force * FrameScale;
  Velocity = Velocity.RotateVector(Step.VelocityRotation);
  Velocity *= std::pow(Friction, FrameScale);

  if (Velocity.SizeSquared() <= 0.001f) {
    Velocity = FVector2D::ZeroVector();
  }

  ApplyMovementOffset(Velocity * FrameScale);
}

void MMovementComponent::ApplyMovementOffset(const FVector2D& Offset) {
  AActor* OwnerActor = GetOwner();
  if (!OwnerActor || Offset.SizeSquared() <= 0.0001f) return;
  OwnerActor->AddActorWorldOffset(Offset);
}

void MMovementComponent::ClearPendingMovement() {
  PendingForce = FVector2D::ZeroVector();
  PendingVelocityRotation = FRotator(0.0f);
}
