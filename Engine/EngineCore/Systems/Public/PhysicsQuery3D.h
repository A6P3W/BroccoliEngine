#pragma once

#include <cstdint>
#include <vector>

#include "BroccoliEngineAPI.h"
#include "UMath.h"

class AActor;

enum class EPhysicsQueryLayer3D : uint8_t { Gameplay, EditorPicking };

enum class EEditorPickingShape3D : uint8_t { Box, Sphere };

struct FEditorPickingProxy3D {
  EEditorPickingShape3D Shape = EEditorPickingShape3D::Sphere;
  FVector3D Center;
  FVector3D HalfExtent;
  float Radius = 0.5f;
};

struct FPhysicsQueryFilter3D {
  uint16_t CollisionMask = 0xffff;
  const AActor* IgnoredActor = nullptr;
  EPhysicsQueryLayer3D QueryLayer = EPhysicsQueryLayer3D::Gameplay;
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
