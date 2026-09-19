#pragma once

#include <cstdint>
#include <memory>

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

 private:
  std::unique_ptr<JPH::PhysicsSystem> PhysicsSystem;
  std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
  std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;
  std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> BroadPhaseLayerInterface;
  std::unique_ptr<JPH::ObjectLayerPairFilterTable> ObjectLayerPairFilter;
  std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> ObjectVsBroadPhaseLayerFilter;
};
