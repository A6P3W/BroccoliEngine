#include <Jolt/Jolt.h>

#include "JoltPhysicsBackend.h"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <thread>

#include "JoltRuntime.h"
#include "Log.h"

namespace {
constexpr JPH::ObjectLayer DefaultObjectLayer = 0;
constexpr uint32_t ObjectLayerCount = 1;
constexpr uint32_t BroadPhaseLayerCount = 1;
constexpr uint32_t MaxBodies = 1024;
constexpr uint32_t BodyMutexCount = 0;
constexpr uint32_t MaxBodyPairs = 1024;
constexpr uint32_t MaxContactConstraints = 1024;
constexpr size_t TempAllocatorSize = 10 * 1024 * 1024;
constexpr uint32_t MaxPhysicsJobs = 1024;
constexpr uint32_t MaxPhysicsBarriers = 1024;
}  // namespace

FJoltPhysicsBackend::FJoltPhysicsBackend() {
  if (!FJoltRuntime::IsInitialized()) {
    M_LOG(Error, "Jolt physics backend was created before the Jolt runtime initialized.");
    return;
  }

  BroadPhaseLayerInterface =
      std::make_unique<JPH::BroadPhaseLayerInterfaceTable>(ObjectLayerCount, BroadPhaseLayerCount);
  BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(DefaultObjectLayer, JPH::BroadPhaseLayer(0));
  ObjectLayerPairFilter = std::make_unique<JPH::ObjectLayerPairFilterTable>(ObjectLayerCount);
  ObjectLayerPairFilter->EnableCollision(DefaultObjectLayer, DefaultObjectLayer);
  ObjectVsBroadPhaseLayerFilter = std::make_unique<JPH::ObjectVsBroadPhaseLayerFilterTable>(
      *BroadPhaseLayerInterface, BroadPhaseLayerCount, *ObjectLayerPairFilter, ObjectLayerCount
  );
  TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(TempAllocatorSize);
  const unsigned int HardwareThreads = std::thread::hardware_concurrency();
  const int WorkerThreadCount = static_cast<int>((std::max)(1u, HardwareThreads)) - 1;
  JobSystem = std::make_unique<JPH::JobSystemThreadPool>(
      MaxPhysicsJobs, MaxPhysicsBarriers, WorkerThreadCount
  );
  PhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
  PhysicsSystem->Init(
      MaxBodies,
      BodyMutexCount,
      MaxBodyPairs,
      MaxContactConstraints,
      *BroadPhaseLayerInterface,
      *ObjectVsBroadPhaseLayerFilter,
      *ObjectLayerPairFilter
  );
}

FJoltPhysicsBackend::~FJoltPhysicsBackend() = default;

bool FJoltPhysicsBackend::Step(float DeltaTime) {
  if (!IsInitialized() || DeltaTime <= 0.0f) {
    return false;
  }

  const JPH::EPhysicsUpdateError UpdateResult =
      PhysicsSystem->Update(DeltaTime, 1, TempAllocator.get(), JobSystem.get());
  if (UpdateResult != JPH::EPhysicsUpdateError::None) {
    M_LOG(
        Error, "Jolt physics step failed with error mask {}.", static_cast<uint32_t>(UpdateResult)
    );
    return false;
  }
  return true;
}

bool FJoltPhysicsBackend::IsInitialized() const {
  return PhysicsSystem != nullptr && TempAllocator != nullptr && JobSystem != nullptr;
}

uint32_t FJoltPhysicsBackend::GetBodyCount() const { return 0; }
