#include "PhysicsSystem3D.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <unordered_map>

#include "Actor.h"
#include "CollisionComponent3D.h"
#include "Jolt/JoltPhysicsBackend.h"
#include "RigidBody3DComponent.h"

namespace {
constexpr float DefaultFixedTimeStep = 1.0f / 60.0f;
constexpr float MaximumFrameDeltaTime = 0.25f;
constexpr uint32_t MaximumStepsPerFrame = 8;

float Dot(const FVector3D& Left, const FVector3D& Right) {
  return Left.X * Right.X + Left.Y * Right.Y + Left.Z * Right.Z;
}

float LengthSquared(const FVector3D& Value) { return Dot(Value, Value); }

FVector3D GetBounds(const MCollisionComponent3D& Collider) {
  if (Collider.GetShapeType3D() == ECollisionShape3D::Sphere) {
    const float Radius = Collider.GetShapeDimensions3D().X;
    return {Radius, Radius, Radius};
  }
  return Collider.GetShapeDimensions3D();
}

bool PassesFilter(
    const AActor& Actor, const MCollisionComponent3D& Collider, const FPhysicsQueryFilter3D& Filter
) {
  if (&Actor == Filter.IgnoredActor || Collider.GetCollisionLayer3D() >= 16) {
    return false;
  }
  return (Filter.CollisionMask & static_cast<uint16_t>(1U << Collider.GetCollisionLayer3D())) != 0;
}
}  // namespace

struct FPhysicsSystem3D::Impl {
  std::unique_ptr<FJoltPhysicsBackend> Backend = std::make_unique<FJoltPhysicsBackend>();
  std::unordered_map<AActor*, MRigidBody3DComponent*> Bodies;
  std::set<std::pair<AActor*, AActor*>> ActivePairs;
  float FixedTimeStep = DefaultFixedTimeStep;
  float Accumulator = 0.0f;
};

FPhysicsSystem3D::FPhysicsSystem3D() : ImplPtr(new Impl()) {}

FPhysicsSystem3D::~FPhysicsSystem3D() { delete ImplPtr; }

void FPhysicsSystem3D::Step(float DeltaTime) {
  if (!IsInitialized() || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
    return;
  }

  ImplPtr->Accumulator += (std::min)(DeltaTime, MaximumFrameDeltaTime);
  uint32_t StepCount = 0;
  while (ImplPtr->Accumulator >= ImplPtr->FixedTimeStep && StepCount < MaximumStepsPerFrame) {
    ImplPtr->Backend->Step(ImplPtr->FixedTimeStep);
    for (const FJoltContactEvent& Event : ImplPtr->Backend->DrainContactEvents()) {
      auto* BodyA =
          static_cast<MRigidBody3DComponent*>(ImplPtr->Backend->FindBodyKey(Event.BodyIdA));
      auto* BodyB =
          static_cast<MRigidBody3DComponent*>(ImplPtr->Backend->FindBodyKey(Event.BodyIdB));
      AActor* ActorA = BodyA ? BodyA->GetOwner() : nullptr;
      AActor* ActorB = BodyB ? BodyB->GetOwner() : nullptr;
      if (!ActorA || !ActorB || ActorA == ActorB || ActorA->IsPendingDestroy() ||
          ActorB->IsPendingDestroy()) {
        continue;
      }
      const std::pair<AActor*, AActor*> Pair =
          ActorA < ActorB ? std::pair{ActorA, ActorB} : std::pair{ActorB, ActorA};
      const std::vector<MCollisionComponent3D*> CollidersA =
          ActorA->GetComponents<MCollisionComponent3D>();
      const std::vector<MCollisionComponent3D*> CollidersB =
          ActorB->GetComponents<MCollisionComponent3D>();
      MCollisionComponent3D* ColliderA = CollidersA.empty() ? nullptr : CollidersA.front();
      MCollisionComponent3D* ColliderB = CollidersB.empty() ? nullptr : CollidersB.front();
      if (!ColliderA || !ColliderB) {
        continue;
      }
      if (Event.Type == FJoltContactEvent::EType::Begin) {
        if (ImplPtr->ActivePairs.insert(Pair).second) {
          ColliderA->NotifyOverlapBegin(ActorB);
          ColliderB->NotifyOverlapBegin(ActorA);
        }
      } else if (ImplPtr->ActivePairs.erase(Pair) > 0) {
        ColliderA->NotifyOverlapEnd(ActorB);
        ColliderB->NotifyOverlapEnd(ActorA);
      }
    }
    for (const auto& [Actor, Body] : ImplPtr->Bodies) {
      if (!Actor || !Body || Body->GetBodyType() != ERigidBody3DType::Dynamic) {
        continue;
      }
      FVector3D Location;
      FQuaternion Rotation;
      if (ImplPtr->Backend->GetTransform(Body, Location, Rotation)) {
        Body->SyncTransformFromPhysics(Location, Rotation);
      }
    }
    ImplPtr->Accumulator -= ImplPtr->FixedTimeStep;
    ++StepCount;
  }
  if (StepCount == MaximumStepsPerFrame) {
    ImplPtr->Accumulator = 0.0f;
  }
}

void FPhysicsSystem3D::SetFixedTimeStep(float NewFixedTimeStep) {
  if (std::isfinite(NewFixedTimeStep) && NewFixedTimeStep > 0.0f) {
    ImplPtr->FixedTimeStep = NewFixedTimeStep;
  }
}

bool FPhysicsSystem3D::IsInitialized() const {
  return ImplPtr != nullptr && ImplPtr->Backend != nullptr && ImplPtr->Backend->IsInitialized();
}

uint32_t FPhysicsSystem3D::GetBodyCount() const {
  return IsInitialized() ? ImplPtr->Backend->GetBodyCount() : 0;
}

float FPhysicsSystem3D::GetFixedTimeStep() const { return ImplPtr->FixedTimeStep; }

void FPhysicsSystem3D::RefreshActorBody(AActor* Actor) {
  if (!Actor || !IsInitialized()) {
    return;
  }
  std::vector<MRigidBody3DComponent*> RigidBodies = Actor->GetComponents<MRigidBody3DComponent>();
  std::vector<MCollisionComponent3D*> Colliders = Actor->GetComponents<MCollisionComponent3D>();
  if (RigidBodies.size() != 1 || Colliders.size() != 1 || !RigidBodies[0]->IsRegistered()) {
    return;
  }

  MRigidBody3DComponent* Body = RigidBodies[0];
  MCollisionComponent3D* Collider = Colliders[0];
  FPhysicsBody3DDesc Description;
  Description.Type = static_cast<EPhysicsBody3DType>(Body->GetBodyType());
  Description.ShapeType = static_cast<EPhysicsShape3DType>(Collider->GetShapeType3D());
  Description.ShapeDimensions = Collider->GetShapeDimensions3D();
  Description.Location = Actor->GetActorLocation3D();
  Description.Rotation = Actor->GetActorRotation3D();
  Description.Mass = Body->GetMass();
  Description.CollisionLayer = Collider->GetCollisionLayer3D();
  Description.CollisionMask = Collider->GetCollisionMask3D();
  Description.bIsSensor = Collider->GetCollisionType3D() == ECollisionType3D::Overlap;
  UnregisterActorBody(Actor);
  if (ImplPtr->Backend->CreateBody(Body, Description)) {
    ImplPtr->Bodies.emplace(Actor, Body);
    Body->SetRegisteredWithPhysics(true);
  }
}

void FPhysicsSystem3D::UnregisterActorBody(AActor* Actor) {
  if (!Actor || !ImplPtr) {
    return;
  }
  const auto It = ImplPtr->Bodies.find(Actor);
  if (It == ImplPtr->Bodies.end()) {
    return;
  }
  for (auto PairIt = ImplPtr->ActivePairs.begin(); PairIt != ImplPtr->ActivePairs.end();) {
    if (PairIt->first != Actor && PairIt->second != Actor) {
      ++PairIt;
      continue;
    }
    AActor* OtherActor = PairIt->first == Actor ? PairIt->second : PairIt->first;
    const std::vector<MCollisionComponent3D*> OtherColliders =
        OtherActor->GetComponents<MCollisionComponent3D>();
    if (!OtherColliders.empty()) {
      MCollisionComponent3D* OtherCollider = OtherColliders.front();
      OtherCollider->RemoveOverlappingActor(Actor);
    }
    PairIt = ImplPtr->ActivePairs.erase(PairIt);
  }
  ImplPtr->Backend->DestroyBody(It->second);
  It->second->SetRegisteredWithPhysics(false);
  ImplPtr->Bodies.erase(It);
}

void FPhysicsSystem3D::SetActorTransform(
    AActor* Actor, const FVector3D& Location, const FQuaternion& Rotation
) {
  const auto It = ImplPtr->Bodies.find(Actor);
  if (It != ImplPtr->Bodies.end()) {
    ImplPtr->Backend->SetTransform(It->second, Location, Rotation);
  }
}

FVector3D FPhysicsSystem3D::GetLinearVelocity(const MRigidBody3DComponent* Component) const {
  return Component
             ? ImplPtr->Backend->GetLinearVelocity(const_cast<MRigidBody3DComponent*>(Component))
             : FVector3D::ZeroVector();
}

void FPhysicsSystem3D::SetLinearVelocity(
    MRigidBody3DComponent* Component, const FVector3D& Velocity
) {
  if (Component) {
    ImplPtr->Backend->SetLinearVelocity(Component, Velocity);
  }
}

void FPhysicsSystem3D::AddForce(MRigidBody3DComponent* Component, const FVector3D& Force) {
  if (Component) {
    ImplPtr->Backend->AddForce(Component, Force);
  }
}

void FPhysicsSystem3D::AddImpulse(MRigidBody3DComponent* Component, const FVector3D& Impulse) {
  if (Component) {
    ImplPtr->Backend->AddImpulse(Component, Impulse);
  }
}

std::vector<FPhysicsQueryHit3D> FPhysicsSystem3D::RaycastAll(
    const FPhysicsRay3D& Ray, const FPhysicsQueryFilter3D& Filter
) const {
  std::vector<FPhysicsQueryHit3D> Hits;
  if (!ImplPtr || Ray.MaxDistance < 0.0f || LengthSquared(Ray.Direction) <= 0.0f) {
    return Hits;
  }
  const float DirectionLength = std::sqrt(LengthSquared(Ray.Direction));
  const FVector3D Direction{
      Ray.Direction.X / DirectionLength,
      Ray.Direction.Y / DirectionLength,
      Ray.Direction.Z / DirectionLength
  };
  for (const auto& [Actor, Body] : ImplPtr->Bodies) {
    if (!Actor || !Body || Actor->IsPendingDestroy()) {
      continue;
    }
    const std::vector<MCollisionComponent3D*> Colliders =
        Actor->GetComponents<MCollisionComponent3D>();
    if (Colliders.empty() || !PassesFilter(*Actor, *Colliders.front(), Filter)) {
      continue;
    }
    const FVector3D Center = Actor->GetActorLocation3D();
    const FVector3D Offset{
        Center.X - Ray.Origin.X, Center.Y - Ray.Origin.Y, Center.Z - Ray.Origin.Z
    };
    const float Distance = Dot(Offset, Direction);
    if (Distance < 0.0f || Distance > Ray.MaxDistance) {
      continue;
    }
    const FVector3D Closest{
        Ray.Origin.X + Direction.X * Distance,
        Ray.Origin.Y + Direction.Y * Distance,
        Ray.Origin.Z + Direction.Z * Distance
    };
    const FVector3D Delta{Center.X - Closest.X, Center.Y - Closest.Y, Center.Z - Closest.Z};
    if (LengthSquared(Delta) <= LengthSquared(GetBounds(*Colliders.front()))) {
      Hits.push_back({Actor, Closest, Distance});
    }
  }
  std::sort(Hits.begin(), Hits.end(), [](const auto& Left, const auto& Right) {
    return Left.Distance < Right.Distance;
  });
  return Hits;
}

bool FPhysicsSystem3D::RaycastNearest(
    const FPhysicsRay3D& Ray, const FPhysicsQueryFilter3D& Filter, FPhysicsQueryHit3D& OutHit
) const {
  std::vector<FPhysicsQueryHit3D> Hits = RaycastAll(Ray, Filter);
  if (Hits.empty()) {
    return false;
  }
  OutHit = Hits.front();
  return true;
}

std::vector<FPhysicsQueryHit3D> FPhysicsSystem3D::OverlapBox(
    const FVector3D& Center, const FVector3D& HalfExtent, const FPhysicsQueryFilter3D& Filter
) const {
  std::vector<FPhysicsQueryHit3D> Hits;
  if (!ImplPtr) {
    return Hits;
  }
  for (const auto& [Actor, Body] : ImplPtr->Bodies) {
    const std::vector<MCollisionComponent3D*> Colliders =
        Actor ? Actor->GetComponents<MCollisionComponent3D>()
              : std::vector<MCollisionComponent3D*>();
    if (!Actor || !Body || Actor->IsPendingDestroy() || Colliders.empty() ||
        !PassesFilter(*Actor, *Colliders.front(), Filter)) {
      continue;
    }
    const FVector3D Bounds = GetBounds(*Colliders.front());
    const FVector3D Location = Actor->GetActorLocation3D();
    if (std::abs(Location.X - Center.X) <= Bounds.X + HalfExtent.X &&
        std::abs(Location.Y - Center.Y) <= Bounds.Y + HalfExtent.Y &&
        std::abs(Location.Z - Center.Z) <= Bounds.Z + HalfExtent.Z) {
      Hits.push_back({Actor, Location, std::sqrt(LengthSquared(Location - Center))});
    }
  }
  return Hits;
}

std::vector<FPhysicsQueryHit3D> FPhysicsSystem3D::OverlapSphere(
    const FVector3D& Center, float Radius, const FPhysicsQueryFilter3D& Filter
) const {
  return OverlapBox(Center, {Radius, Radius, Radius}, Filter);
}
