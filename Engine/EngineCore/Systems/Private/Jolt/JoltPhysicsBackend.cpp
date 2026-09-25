// clang-format off
#include <Jolt/Jolt.h>
// clang-format on

#include "JoltPhysicsBackend.h"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <thread>

#include "JoltContactListener.h"
#include "JoltRuntime.h"
#include "Log.h"

namespace {
constexpr JPH::ObjectLayer GameplayObjectLayer = 0;
constexpr JPH::ObjectLayer EditorPickingObjectLayer = 1;
constexpr uint32_t ObjectLayerCount = 2;
constexpr uint32_t BroadPhaseLayerCount = 2;
constexpr uint32_t MaxBodies = 1024;
constexpr uint32_t BodyMutexCount = 0;
constexpr uint32_t MaxBodyPairs = 1024;
constexpr uint32_t MaxContactConstraints = 1024;
constexpr size_t TempAllocatorSize = 10 * 1024 * 1024;
constexpr uint32_t MaxPhysicsJobs = 1024;
constexpr uint32_t MaxPhysicsBarriers = 1024;

class FQueryObjectLayerFilter final : public JPH::ObjectLayerFilter {
 public:
  explicit FQueryObjectLayerFilter(JPH::ObjectLayer InLayer) : Layer(InLayer) {}

  bool ShouldCollide(JPH::ObjectLayer Candidate) const override { return Candidate == Layer; }

 private:
  JPH::ObjectLayer Layer;
};
}  // namespace

struct FJoltPhysicsBackend::FBodyRecord {
  JPH::BodyID Id;
  uint16_t CollisionLayer = 0;
  uint16_t CollisionMask = 0xffff;
};

std::vector<void*> FJoltPhysicsBackend::OverlapShape(
    const FVector3D& Center, const FVector3D& Dimensions, bool Sphere
) const {
  std::vector<void*> Result;
  if (!IsInitialized() || !std::isfinite(Center.X) || !std::isfinite(Center.Y) ||
      !std::isfinite(Center.Z) || !std::isfinite(Dimensions.X) || !std::isfinite(Dimensions.Y) ||
      !std::isfinite(Dimensions.Z) || Dimensions.X <= 0 || Dimensions.Y <= 0 || Dimensions.Z <= 0) {
    return Result;
  }
  JPH::ShapeSettings::ShapeResult ShapeResult;
  if (Sphere) {
    ShapeResult = JPH::SphereShapeSettings(Dimensions.X).Create();
  } else {
    ShapeResult =
        JPH::BoxShapeSettings(JPH::Vec3(Dimensions.X, Dimensions.Y, Dimensions.Z), 0.0f).Create();
  }
  if (ShapeResult.HasError()) {
    return Result;
  }
  JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> Collector;
  const JPH::RVec3 Position(Center.X, Center.Y, Center.Z);
  PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
      ShapeResult.Get(),
      JPH::Vec3::sReplicate(1.0f),
      JPH::RMat44::sTranslation(Position),
      JPH::CollideShapeSettings(),
      Position,
      Collector
  );
  for (const auto& Hit : Collector.mHits) {
    void* Key = FindBodyKey(Hit.mBodyID2.GetIndexAndSequenceNumber());
    if (Key && std::find(Result.begin(), Result.end(), Key) == Result.end()) {
      Result.push_back(Key);
    }
  }
  return Result;
}

std::vector<FJoltRaycastHit> FJoltPhysicsBackend::RaycastAll(
    const FVector3D& Origin,
    const FVector3D& Direction,
    float MaxDistance,
    EPhysicsQueryLayer3D Layer
) const {
  std::vector<FJoltRaycastHit> Result;
  if (!IsInitialized() || !std::isfinite(Origin.X) || !std::isfinite(Origin.Y) ||
      !std::isfinite(Origin.Z) || !std::isfinite(Direction.X) || !std::isfinite(Direction.Y) ||
      !std::isfinite(Direction.Z) || !std::isfinite(MaxDistance) || MaxDistance < 0.0f) {
    return Result;
  }
  const float LengthSquared =
      Direction.X * Direction.X + Direction.Y * Direction.Y + Direction.Z * Direction.Z;
  if (LengthSquared <= 0.0f) {
    return Result;
  }
  const float InverseLength = 1.0f / std::sqrt(LengthSquared);
  const JPH::RRayCast Ray(
      JPH::RVec3(Origin.X, Origin.Y, Origin.Z),
      JPH::Vec3(
          Direction.X * InverseLength * MaxDistance,
          Direction.Y * InverseLength * MaxDistance,
          Direction.Z * InverseLength * MaxDistance
      )
  );
  JPH::AllHitCollisionCollector<JPH::CastRayCollector> Collector;
  const FQueryObjectLayerFilter LayerFilter(
      Layer == EPhysicsQueryLayer3D::EditorPicking ? EditorPickingObjectLayer : GameplayObjectLayer
  );
  PhysicsSystem->GetNarrowPhaseQuery().CastRay(
      Ray, JPH::RayCastSettings(), Collector, {}, LayerFilter
  );
  Collector.Sort();
  for (const JPH::RayCastResult& Hit : Collector.mHits) {
    void* Key = FindBodyKey(Hit.mBodyID.GetIndexAndSequenceNumber());
    if (Key) {
      Result.push_back({Key, Hit.mFraction});
    }
  }
  return Result;
}

namespace {
class FJoltContactListener final : public JPH::ContactListener {
 public:
  FJoltContactListener(
      FJoltContactEventQueue& InEvents, std::function<bool(uint32_t, uint32_t)> InShouldCollide
  )
      : Events(InEvents), ShouldCollide(std::move(InShouldCollide)) {}

  JPH::ValidateResult OnContactValidate(
      const JPH::Body& BodyA, const JPH::Body& BodyB, JPH::RVec3Arg, const JPH::CollideShapeResult&
  ) override {
    return ShouldCollide(
               BodyA.GetID().GetIndexAndSequenceNumber(), BodyB.GetID().GetIndexAndSequenceNumber()
           )
               ? JPH::ValidateResult::AcceptAllContactsForThisBodyPair
               : JPH::ValidateResult::RejectAllContactsForThisBodyPair;
  }

  void OnContactAdded(
      const JPH::Body& BodyA,
      const JPH::Body& BodyB,
      const JPH::ContactManifold&,
      JPH::ContactSettings&
  ) override {
    Events.Push(
        BodyA.GetID().GetIndexAndSequenceNumber(),
        BodyB.GetID().GetIndexAndSequenceNumber(),
        FJoltContactEvent::EType::Begin
    );
  }

  void OnContactRemoved(const JPH::SubShapeIDPair& Pair) override {
    Events.Push(
        Pair.GetBody1ID().GetIndexAndSequenceNumber(),
        Pair.GetBody2ID().GetIndexAndSequenceNumber(),
        FJoltContactEvent::EType::End
    );
  }

 private:
  FJoltContactEventQueue& Events;
  std::function<bool(uint32_t, uint32_t)> ShouldCollide;
};
}  // namespace

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
  BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(
      GameplayObjectLayer, JPH::BroadPhaseLayer(0)
  );
  BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(
      EditorPickingObjectLayer, JPH::BroadPhaseLayer(1)
  );
  ObjectLayerPairFilter = std::make_unique<JPH::ObjectLayerPairFilterTable>(ObjectLayerCount);
  ObjectLayerPairFilter->EnableCollision(GameplayObjectLayer, GameplayObjectLayer);
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
  ContactEvents = std::make_unique<FJoltContactEventQueue>();
  ContactListener = std::make_unique<FJoltContactListener>(
      *ContactEvents,
      [this](uint32_t BodyIdA, uint32_t BodyIdB) { return ShouldCollide(BodyIdA, BodyIdB); }
  );
  PhysicsSystem->SetContactListener(ContactListener.get());
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

bool FJoltPhysicsBackend::CreateBody(
    void* Key, const FPhysicsBody3DDesc& Description, EPhysicsQueryLayer3D Layer
) {
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
      Layer == EPhysicsQueryLayer3D::EditorPicking ? EditorPickingObjectLayer : GameplayObjectLayer
  );
  Settings.mIsSensor = Description.bIsSensor;
  if (Description.Type == EPhysicsBody3DType::Dynamic && Description.Mass > 0.0f) {
    Settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    Settings.mMassPropertiesOverride.mMass = Description.Mass;
  }
  JPH::BodyInterface& BodyInterface = PhysicsSystem->GetBodyInterface();
  const JPH::BodyID Id = BodyInterface.CreateAndAddBody(Settings, JPH::EActivation::Activate);
  if (Id.IsInvalid()) {
    return false;
  }
  Bodies.emplace(Key, FBodyRecord{Id, Description.CollisionLayer, Description.CollisionMask});
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

bool FJoltPhysicsBackend::MoveKinematic(
    void* Key, const FVector3D& TargetLocation, const FQuaternion& TargetRotation, float DeltaTime
) {
  const auto It = Bodies.find(Key);
  if (It == Bodies.end()) {
    return false;
  }
  PhysicsSystem->GetBodyInterface().MoveKinematic(
      It->second.Id, ToJolt(TargetLocation), ToJolt(TargetRotation), DeltaTime
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

std::vector<FJoltContactEvent> FJoltPhysicsBackend::DrainContactEvents() {
  return ContactEvents ? ContactEvents->Drain() : std::vector<FJoltContactEvent>();
}

void* FJoltPhysicsBackend::FindBodyKey(uint32_t BodyId) const {
  for (const auto& [Key, Record] : Bodies) {
    if (Record.Id.GetIndexAndSequenceNumber() == BodyId) {
      return Key;
    }
  }
  return nullptr;
}

bool FJoltPhysicsBackend::ShouldDispatchContactEnd(uint32_t BodyIdA, uint32_t BodyIdB) const {
  if (!PhysicsSystem) {
    return false;
  }
  const FBodyRecord* RecordA = nullptr;
  const FBodyRecord* RecordB = nullptr;
  for (const auto& [Key, Record] : Bodies) {
    if (Record.Id.GetIndexAndSequenceNumber() == BodyIdA) {
      RecordA = &Record;
    } else if (Record.Id.GetIndexAndSequenceNumber() == BodyIdB) {
      RecordB = &Record;
    }
  }
  if (!RecordA || !RecordB) {
    return false;
  }
  const JPH::BodyInterface& BodyInterface = PhysicsSystem->GetBodyInterface();
  if (!BodyInterface.IsActive(RecordA->Id) && !BodyInterface.IsActive(RecordB->Id)) {
    return false;
  }
  return !PhysicsSystem->WereBodiesInContact(RecordA->Id, RecordB->Id);
}

bool FJoltPhysicsBackend::ShouldCollide(uint32_t BodyIdA, uint32_t BodyIdB) const {
  const FBodyRecord* RecordA = nullptr;
  const FBodyRecord* RecordB = nullptr;
  for (const auto& [Key, Record] : Bodies) {
    if (Record.Id.GetIndexAndSequenceNumber() == BodyIdA) {
      RecordA = &Record;
    } else if (Record.Id.GetIndexAndSequenceNumber() == BodyIdB) {
      RecordB = &Record;
    }
  }
  if (!RecordA || !RecordB || RecordA->CollisionLayer >= 16 || RecordB->CollisionLayer >= 16) {
    return false;
  }
  const uint16_t LayerMaskA = static_cast<uint16_t>(1U << RecordA->CollisionLayer);
  const uint16_t LayerMaskB = static_cast<uint16_t>(1U << RecordB->CollisionLayer);
  return (RecordA->CollisionMask & LayerMaskB) != 0 && (RecordB->CollisionMask & LayerMaskA) != 0;
}
