// clang-format off
#include <Jolt/Jolt.h>
// clang-format on

#include "JoltPhysicsBackend.h"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
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

struct FJoltPhysicsBackend::FBodyRecord {
  JPH::BodyID Id;
};

namespace {
JPH::RVec3 ToJolt(const FVector3D& Value) { return {Value.X, Value.Y, Value.Z}; }

JPH::Quat ToJolt(const FQuaternion& Value) {
  const FQuaternion Unit = Value.Normalize();
  return {Unit.X, Unit.Y, Unit.Z, Unit.W};
}

FVector3D FromJolt(const JPH::RVec3& Value) {
  return {
      static_cast<float>(Value.GetX()),
      static_cast<float>(Value.GetY()),
      static_cast<float>(Value.GetZ())
  };
}

FQuaternion FromJolt(const JPH::Quat& Value) {
  return {Value.GetX(), Value.GetY(), Value.GetZ(), Value.GetW()};
}

JPH::EMotionType ToJolt(EPhysicsBody3DType Type) {
  switch (Type) {
    case EPhysicsBody3DType::Dynamic:
      return JPH::EMotionType::Dynamic;
    case EPhysicsBody3DType::Kinematic:
      return JPH::EMotionType::Kinematic;
    case EPhysicsBody3DType::Static:
    default:
      return JPH::EMotionType::Static;
  }
}
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

FJoltPhysicsBackend::~FJoltPhysicsBackend() {
  if (!PhysicsSystem) {
    return;
  }
  JPH::BodyInterface& BodyInterface = PhysicsSystem->GetBodyInterface();
  for (const auto& [Key, Record] : Bodies) {
    BodyInterface.RemoveBody(Record.Id);
    BodyInterface.DestroyBody(Record.Id);
  }
}

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

uint32_t FJoltPhysicsBackend::GetBodyCount() const { return static_cast<uint32_t>(Bodies.size()); }

bool FJoltPhysicsBackend::CreateBody(void* Key, const FPhysicsBody3DDesc& Description) {
  if (!Key || !IsInitialized()) {
    return false;
  }
  DestroyBody(Key);

  JPH::ShapeSettings::ShapeResult ShapeResult;
  if (Description.ShapeType == EPhysicsShape3DType::Box) {
    ShapeResult = JPH::BoxShapeSettings(ToJolt(Description.ShapeDimensions)).Create();
  } else {
    ShapeResult = JPH::SphereShapeSettings(Description.ShapeDimensions.X).Create();
  }
  if (ShapeResult.HasError()) {
    M_LOG(Error, "Failed to create Jolt collision shape: {}", ShapeResult.GetError());
    return false;
  }

  JPH::BodyCreationSettings Settings(
      ShapeResult.Get(),
      ToJolt(Description.Location),
      ToJolt(Description.Rotation),
      ToJolt(Description.Type),
      DefaultObjectLayer
  );
  if (Description.Type == EPhysicsBody3DType::Dynamic && Description.Mass > 0.0f) {
    Settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    Settings.mMassPropertiesOverride.mMass = Description.Mass;
  }
  JPH::BodyInterface& BodyInterface = PhysicsSystem->GetBodyInterface();
  const JPH::BodyID Id = BodyInterface.CreateAndAddBody(Settings, JPH::EActivation::Activate);
  if (Id.IsInvalid()) {
    return false;
  }
  Bodies.emplace(Key, FBodyRecord{Id});
  return true;
}

void FJoltPhysicsBackend::DestroyBody(void* Key) {
  const auto It = Bodies.find(Key);
  if (It == Bodies.end() || !PhysicsSystem) {
    return;
  }
  JPH::BodyInterface& BodyInterface = PhysicsSystem->GetBodyInterface();
  BodyInterface.RemoveBody(It->second.Id);
  BodyInterface.DestroyBody(It->second.Id);
  Bodies.erase(It);
}

bool FJoltPhysicsBackend::SetTransform(
    void* Key, const FVector3D& Location, const FQuaternion& Rotation
) {
  const auto It = Bodies.find(Key);
  if (It == Bodies.end()) {
    return false;
  }
  PhysicsSystem->GetBodyInterface().SetPositionAndRotation(
      It->second.Id, ToJolt(Location), ToJolt(Rotation), JPH::EActivation::Activate
  );
  return true;
}

bool FJoltPhysicsBackend::GetTransform(
    void* Key, FVector3D& OutLocation, FQuaternion& OutRotation
) const {
  const auto It = Bodies.find(Key);
  if (It == Bodies.end()) {
    return false;
  }
  JPH::RVec3 Location;
  JPH::Quat Rotation;
  PhysicsSystem->GetBodyInterface().GetPositionAndRotation(It->second.Id, Location, Rotation);
  OutLocation = FromJolt(Location);
  OutRotation = FromJolt(Rotation);
  return true;
}

FVector3D FJoltPhysicsBackend::GetLinearVelocity(void* Key) const {
  const auto It = Bodies.find(Key);
  return It == Bodies.end()
             ? FVector3D::ZeroVector()
             : FromJolt(PhysicsSystem->GetBodyInterface().GetLinearVelocity(It->second.Id));
}

void FJoltPhysicsBackend::SetLinearVelocity(void* Key, const FVector3D& Velocity) {
  const auto It = Bodies.find(Key);
  if (It != Bodies.end()) {
    PhysicsSystem->GetBodyInterface().SetLinearVelocity(It->second.Id, ToJolt(Velocity));
  }
}

void FJoltPhysicsBackend::AddForce(void* Key, const FVector3D& Force) {
  const auto It = Bodies.find(Key);
  if (It != Bodies.end()) {
    PhysicsSystem->GetBodyInterface().AddForce(It->second.Id, ToJolt(Force));
  }
}

void FJoltPhysicsBackend::AddImpulse(void* Key, const FVector3D& Impulse) {
  const auto It = Bodies.find(Key);
  if (It != Bodies.end()) {
    PhysicsSystem->GetBodyInterface().AddImpulse(It->second.Id, ToJolt(Impulse));
  }
}
