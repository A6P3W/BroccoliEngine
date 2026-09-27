#include "MovingPlatformActor.h"

#include <cmath>

REGISTER_ACTOR(AMovingPlatformActor)

void AMovingPlatformActor::BeginPlay() {
  AActor::BeginPlay();
  StartLocation = GetActorLocation3D();
  Travel = 0.0f;
}

void AMovingPlatformActor::OnUpdate(float DeltaTime) {
  if (!Enabled || DeltaTime <= 0.0f || Speed <= 0.0f || Distance <= 0.0f) return;

  const FVector3D UnitDirection = Direction.Normalize();
  if (UnitDirection.SizeSquared() == 0.0f) return;

  const float Period = Distance * 2.0f;
  Travel = std::fmod(Travel + Speed * DeltaTime, Period);
  const float Offset = Travel <= Distance ? Travel : Period - Travel;
  SetActorLocation3D(StartLocation + UnitDirection * Offset);
}
