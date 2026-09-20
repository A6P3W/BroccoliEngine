#pragma once

#include <cstdint>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "UMath.h"

class AActor;

struct FPhysicsQueryFilter3D {
  uint16_t CollisionMask = 0xffff;
  const AActor* IgnoredActor = nullptr;
};

struct FPhysicsRay3D {
  FVector3D Origin;
  FVector3D Direction{0.0f, 1.0f, 0.0f};
  float MaxDistance = 1000.0f;
};

struct FPhysicsQueryHit3D {
  AActor* Actor = nullptr;
  FVector3D Location;
  float Distance = 0.0f;
};
