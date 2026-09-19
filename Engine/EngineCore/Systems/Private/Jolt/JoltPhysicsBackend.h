#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "PhysicsSystem3D.h"

namespace JPH {
class PhysicsSystem;
class TempAllocatorImpl;
class JobSystemThreadPool;
class BroadPhaseLayerInterfaceTable;
class ObjectLayerPairFilterTable;
class ObjectVsBroadPhaseLayerFilterTable;
}  // namespace JPH

class FJoltPhysicsBackend {
 public:
  FJoltPhysicsBackend();
  ~FJoltPhysicsBackend();
  FJoltPhysicsBackend(const FJoltPhysicsBackend&) = delete;
  FJoltPhysicsBackend& operator=(const FJoltPhysicsBackend&) = delete;

  bool Step(float DeltaTime);
  bool IsInitialized() const;
  uint32_t GetBodyCount() const;
  bool CreateBody(void* Key, const FPhysicsBody3DDesc& Description);
  void DestroyBody(void* Key);
  bool SetTransform(void* Key, const FVector3D& Location, const FQuaternion& Rotation);
  bool GetTransform(void* Key, FVector3D& OutLocation, FQuaternion& OutRotation) const;
  FVector3D GetLinearVelocity(void* Key) const;
  void SetLinearVelocity(void* Key, const FVector3D& Velocity);
  void AddForce(void* Key, const FVector3D& Force);
  void AddImpulse(void* Key, const FVector3D& Impulse);

 private:
  std::unique_ptr<JPH::PhysicsSystem> PhysicsSystem;
  std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
  std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;
  std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> BroadPhaseLayerInterface;
  std::unique_ptr<JPH::ObjectLayerPairFilterTable> ObjectLayerPairFilter;
  std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> ObjectVsBroadPhaseLayerFilter;
  struct FBodyRecord;
  std::unordered_map<void*, FBodyRecord> Bodies;
};
