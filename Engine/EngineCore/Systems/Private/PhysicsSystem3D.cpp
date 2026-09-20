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
