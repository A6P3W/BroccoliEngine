#pragma once
#include "Utils/UMath.h"
#include "ActorComponent.h"
class MMovementComponent : public MActorComponent
{
public:
	void AddForce(const FVector2D& Force);

	void OnUpdate(float DeltaTime) override;
private:
	FVector2D Velocity;
	float Friction = 0.98f;
};

