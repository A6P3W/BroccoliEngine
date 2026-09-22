#include "Physics3DBodyTestActor.h"

#include <algorithm>

#include "BoxCollision3DComponent.h"
#include "ControlMacros.h"
#include "CubeComponent.h"
#include "RigidBody3DComponent.h"
#include "SphereCollision3DComponent.h"

REGISTER_ACTOR(APhysics3DStaticFloorActor)
REGISTER_ACTOR(APhysics3DDynamicBoxActor)
REGISTER_ACTOR(APhysics3DDynamicSphereActor)
REGISTER_ACTOR(APhysics3DKinematicActor)
CONTROL_METHOD(
    "get_collision_observation",
    "Returns 3D overlap event counts and current overlap actor IDs.",
    &APhysics3DDynamicBoxActor::GetCollisionObservation,
    CONTROL_PARAMETERS(),
    ([](const FPhysics3DCollisionObservation& Observation) {
      return nlohmann::json{
          {"begin_overlap_count", Observation.BeginOverlapCount},
          {"end_overlap_count", Observation.EndOverlapCount},
          {"overlapping_actor_ids", Observation.OverlappingActorIds}
      };
    })
)
CONTROL_METHOD(
    "get_collision_observation",
    "Returns 3D overlap event counts and current overlap actor IDs.",
    &APhysics3DDynamicSphereActor::GetCollisionObservation,
    CONTROL_PARAMETERS(),
    ([](const FPhysics3DCollisionObservation& Observation) {
      return nlohmann::json{
          {"begin_overlap_count", Observation.BeginOverlapCount},
          {"end_overlap_count", Observation.EndOverlapCount},
          {"overlapping_actor_ids", Observation.OverlappingActorIds}
      };
    })
)

namespace {
void ConfigureBody(AActor& Actor, ERigidBody3DType Type) {
  MRigidBody3DComponent* Body = NewObject<MRigidBody3DComponent>(&Actor);
  Body->SetBodyType(Type);
  Body->RegisterComponent();
}
}  // namespace

APhysics3DStaticFloorActor::APhysics3DStaticFloorActor() {
  ConfigureBody(*this, ERigidBody3DType::Static);
  MBoxCollision3DComponent* Collider = NewObject<MBoxCollision3DComponent>(this);
  Collider->SetHalfExtent({5.0f, 0.5f, 5.0f});
  Collider->RegisterComponent();

  MCubeComponent* Visual = NewObject<MCubeComponent>(this);
  Visual->SetRelativeScale3D({10.0f, 1.0f, 10.0f});
  Visual->SetColor({130, 130, 130, 255});
  Visual->RegisterComponent();
}

void APhysics3DStaticFloorActor::BeginPlay() { SetActorLocation3D({0.0f, -1.0f, 0.0f}); }

APhysics3DDynamicBoxActor::APhysics3DDynamicBoxActor() {
  ConfigureBody(*this, ERigidBody3DType::Dynamic);
  MBoxCollision3DComponent* Collider = NewObject<MBoxCollision3DComponent>(this);
  Collider->SetHalfExtent({0.5f, 0.5f, 0.5f});
  Collider->RegisterComponent();

  MCubeComponent* Visual = NewObject<MCubeComponent>(this);
  Visual->SetRelativeScale3D({1.0f, 1.0f, 1.0f});
  Visual->SetColor({230, 80, 80, 255});
  Visual->RegisterComponent();
}

void APhysics3DDynamicBoxActor::BeginPlay() { SetActorLocation3D({-2.0f, 5.0f, 0.0f}); }

FPhysics3DCollisionObservation APhysics3DDynamicBoxActor::GetCollisionObservation() const {
  return {BeginOverlapCount, EndOverlapCount, OverlappingActorIds};
}

void APhysics3DDynamicBoxActor::BeginOverlap(AActor* OtherActor) {
  if (!OtherActor) {
    return;
  }
  ++BeginOverlapCount;
  OverlappingActorIds.push_back(OtherActor->GetActorId());
}

void APhysics3DDynamicBoxActor::EndOverlap(AActor* OtherActor) {
  if (!OtherActor) {
    return;
  }
  ++EndOverlapCount;
  std::erase(OverlappingActorIds, OtherActor->GetActorId());
}

APhysics3DDynamicSphereActor::APhysics3DDynamicSphereActor() {
  ConfigureBody(*this, ERigidBody3DType::Dynamic);
  MSphereCollision3DComponent* Collider = NewObject<MSphereCollision3DComponent>(this);
  Collider->SetRadius(0.5f);
  Collider->RegisterComponent();

  MCubeComponent* Visual = NewObject<MCubeComponent>(this);
  Visual->SetRelativeScale3D({1.0f, 1.0f, 1.0f});
  Visual->SetColor({80, 160, 240, 255});
  Visual->RegisterComponent();
}

void APhysics3DDynamicSphereActor::BeginPlay() { SetActorLocation3D({2.0f, 5.0f, 0.0f}); }

FPhysics3DCollisionObservation APhysics3DDynamicSphereActor::GetCollisionObservation() const {
  return {BeginOverlapCount, EndOverlapCount, OverlappingActorIds};
}

void APhysics3DDynamicSphereActor::BeginOverlap(AActor* OtherActor) {
  if (!OtherActor) {
    return;
  }
  ++BeginOverlapCount;
  OverlappingActorIds.push_back(OtherActor->GetActorId());
}

void APhysics3DDynamicSphereActor::EndOverlap(AActor* OtherActor) {
  if (!OtherActor) {
    return;
  }
  ++EndOverlapCount;
  std::erase(OverlappingActorIds, OtherActor->GetActorId());
}

APhysics3DKinematicActor::APhysics3DKinematicActor() {
  ConfigureBody(*this, ERigidBody3DType::Kinematic);
  MBoxCollision3DComponent* Collider = NewObject<MBoxCollision3DComponent>(this);
  Collider->SetHalfExtent({0.5f, 0.5f, 0.5f});
  Collider->RegisterComponent();

  MCubeComponent* Visual = NewObject<MCubeComponent>(this);
  Visual->SetRelativeScale3D({1.0f, 1.0f, 1.0f});
  Visual->SetColor({80, 220, 120, 255});
  Visual->RegisterComponent();
}

void APhysics3DKinematicActor::BeginPlay() { SetActorLocation3D({0.0f, 2.0f, 3.0f}); }
