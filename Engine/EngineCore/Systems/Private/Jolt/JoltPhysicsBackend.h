#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "JoltContactListener.h"
#include "PhysicsSystem3D.h"

namespace JPH {
class PhysicsSystem;
class TempAllocatorImpl;
class JobSystemThreadPool;
class BroadPhaseLayerInterfaceTable;
class ObjectLayerPairFilterTable;
class ObjectVsBroadPhaseLayerFilterTable;
class ContactListener;
}  // namespace JPH

struct FJoltRaycastHit {
  void* Key = nullptr;
  float Fraction = 0.0f;
};

class FJoltPhysicsBackend {
 public:
  FJoltPhysicsBackend();
  ~FJoltPhysicsBackend();
  FJoltPhysicsBackend(const FJoltPhysicsBackend&) = delete;
  FJoltPhysicsBackend& operator=(const FJoltPhysicsBackend&) = delete;

  bool Step(float DeltaTime);
  bool IsInitialized() const;
  uint32_t GetBodyCount() const;
  bool CreateBody(
      void* Key,
      const FPhysicsBody3DDesc& Description,
      EPhysicsQueryLayer3D Layer = EPhysicsQueryLayer3D::Gameplay
  );
  void DestroyBody(void* Key);
  bool SetTransform(void* Key, const FVector3D& Location, const FQuaternion& Rotation);
  bool MoveKinematic(
      void* Key, const FVector3D& TargetLocation, const FQuaternion& TargetRotation, float DeltaTime
  );
  bool GetTransform(void* Key, FVector3D& OutLocation, FQuaternion& OutRotation) const;
  FVector3D GetLinearVelocity(void* Key) const;
  void SetLinearVelocity(void* Key, const FVector3D& Velocity);
  void AddForce(void* Key, const FVector3D& Force);
  void AddImpulse(void* Key, const FVector3D& Impulse);
  std::vector<FJoltContactEvent> DrainContactEvents();
  void* FindBodyKey(uint32_t BodyId) const;
  bool ShouldDispatchContactEnd(uint32_t BodyIdA, uint32_t BodyIdB) const;
  std::vector<void*> OverlapShape(
      const FVector3D& Center, const FVector3D& Dimensions, bool Sphere
  ) const;
  std::vector<FJoltRaycastHit> RaycastAll(
      const FVector3D& Origin, const FVector3D& Direction, float MaxDistance
  ) const;

 private:
  std::unique_ptr<JPH::PhysicsSystem> PhysicsSystem;
  std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
  std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;
  std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> BroadPhaseLayerInterface;
  std::unique_ptr<JPH::ObjectLayerPairFilterTable> ObjectLayerPairFilter;
  std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> ObjectVsBroadPhaseLayerFilter;
  struct FBodyRecord;
  bool ShouldCollide(uint32_t BodyIdA, uint32_t BodyIdB) const;
  std::unordered_map<void*, FBodyRecord> Bodies;
  std::unique_ptr<FJoltContactEventQueue> ContactEvents;
  std::unique_ptr<JPH::ContactListener> ContactListener;
};
