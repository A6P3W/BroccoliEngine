#include "MovementComponent.h"
#include <SceneComponent.h>
#include <Actor.h>
#include <Utils/UMath.h>
#include <Utils/Log.h>
void MMovementComponent::AddWorldForce(const FVector2D& Force)
{
	Velocity = Velocity + Force;
}

void MMovementComponent::AddLocalForce(const FVector2D& Force)
{

	Velocity = Velocity + Force.RotateVector(GetOwner()->GetActorRotation());
}

void MMovementComponent::SetWorldForce(const FVector2D& Force)
{
	Velocity = Force;
}

void MMovementComponent::SetLocalForce(const FVector2D & Force)
{
	Velocity = Force.RotateVector(GetOwner()->GetActorRotation());
}

void MMovementComponent::AddVelocityRotation(const FRotator& Rotation)
{
	Velocity =Velocity.RotateVector(Rotation);
}

void MMovementComponent::SetVelocityRotation(const FRotator& Rotation)
{
	FRotator currentRotation = GetOwner()->GetActorRotation();
	Velocity = Velocity.RotateVector(Rotation- currentRotation);
}

void MMovementComponent::OnUpdate(float DeltaTime)
{
	MLog::Log("Velocity: " + std::to_string(Velocity.X) + ", " + std::to_string(Velocity.Y));
	if (Velocity.SizeSquared() < 0.1f) {
		Velocity = FVector2D::ZeroVector;
		return;
	}
	if (!GetOwner())return;
	AActor* owner = GetOwner();
	Velocity *= std::pow(Friction, DeltaTime * 60);

	owner->AddActorWorldOffset(Velocity);
}
