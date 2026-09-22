#pragma once

#include <cstdint>

#include "BroccoliEngineAPI.h"
#include "PhysicsQuery3D.h"
#include "UMath.h"

class AActor;
class MRigidBody3DComponent;
class MCollision3DComponent;

enum class EPhysicsBody3DType : uint8_t { Static, Kinematic, Dynamic };
enum class EPhysicsShape3DType : uint8_t { Box, Sphere };

struct FPhysicsBody3DDesc {
  EPhysicsBody3DType Type = EPhysicsBody3DType::Static;
  EPhysicsShape3DType ShapeType = EPhysicsShape3DType::Box;
  FVector3D ShapeDimensions{0.5f, 0.5f, 0.5f};
  FVector3D Location;
  FQuaternion Rotation;
  float Mass = 1.0f;
  uint16_t CollisionLayer = 0;
  uint16_t CollisionMask = 0xffff;
  bool bIsSensor = false;
};

class BROCCOLI_ENGINE_API FPhysicsSystem3D {
 public:
  FPhysicsSystem3D();
  ~FPhysicsSystem3D();
  FPhysicsSystem3D(const FPhysicsSystem3D&) = delete;
  FPhysicsSystem3D& operator=(const FPhysicsSystem3D&) = delete;

  void Step(float DeltaTime);
  void SetFixedTimeStep(float NewFixedTimeStep);
  void RefreshActorBody(AActor* Actor);
  void UnregisterActorBody(AActor* Actor);
  void SetActorTransform(AActor* Actor, const FVector3D& Location, const FQuaternion& Rotation);
  FVector3D GetLinearVelocity(const MRigidBody3DComponent* Component) const;
  void SetLinearVelocity(MRigidBody3DComponent* Component, const FVector3D& Velocity);
  void AddForce(MRigidBody3DComponent* Component, const FVector3D& Force);
  void AddImpulse(MRigidBody3DComponent* Component, const FVector3D& Impulse);
  bool RaycastNearest(
      const FPhysicsRay3D& Ray, const FPhysicsQueryFilter3D& Filter, FPhysicsQueryHit3D& OutHit
  ) const;
  std::vector<FPhysicsQueryHit3D> RaycastAll(
      const FPhysicsRay3D& Ray, const FPhysicsQueryFilter3D& Filter
  ) const;
  std::vector<FPhysicsQueryHit3D> OverlapBox(
      const FVector3D& Center, const FVector3D& HalfExtent, const FPhysicsQueryFilter3D& Filter
  ) const;
  std::vector<FPhysicsQueryHit3D> OverlapSphere(
      const FVector3D& Center, float Radius, const FPhysicsQueryFilter3D& Filter
  ) const;

  bool IsInitialized() const;
  uint32_t GetBodyCount() const;
  float GetFixedTimeStep() const;

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
