#include "Physics3DBodyTestActor.h"

#include "BoxCollisionComponent3D.h"
#include "RigidBody3DComponent.h"
#include "SphereCollisionComponent3D.h"

REGISTER_ACTOR(APhysics3DStaticFloorActor)
REGISTER_ACTOR(APhysics3DDynamicBoxActor)
REGISTER_ACTOR(APhysics3DDynamicSphereActor)
REGISTER_ACTOR(APhysics3DKinematicActor)

namespace {
void ConfigureBody(AActor& Actor, ERigidBody3DType Type) {
  MRigidBody3DComponent* Body = NewObject<MRigidBody3DComponent>(&Actor);
  Body->SetBodyType(Type);
  Body->RegisterComponent();
}
}  // namespace

APhysics3DStaticFloorActor::APhysics3DStaticFloorActor() {
  SetActorLocation3D({0.0f, -1.0f, 0.0f});
  ConfigureBody(*this, ERigidBody3DType::Static);
  MBoxCollisionComponent3D* Collider = NewObject<MBoxCollisionComponent3D>(this);
  Collider->SetHalfExtent({5.0f, 0.5f, 5.0f});
  Collider->RegisterComponent();
}

APhysics3DDynamicBoxActor::APhysics3DDynamicBoxActor() {
  SetActorLocation3D({-2.0f, 5.0f, 0.0f});
  ConfigureBody(*this, ERigidBody3DType::Dynamic);
  MBoxCollisionComponent3D* Collider = NewObject<MBoxCollisionComponent3D>(this);
  Collider->SetHalfExtent({0.5f, 0.5f, 0.5f});
  Collider->RegisterComponent();
}

APhysics3DDynamicSphereActor::APhysics3DDynamicSphereActor() {
  SetActorLocation3D({2.0f, 5.0f, 0.0f});
  ConfigureBody(*this, ERigidBody3DType::Dynamic);
  MSphereCollisionComponent3D* Collider = NewObject<MSphereCollisionComponent3D>(this);
  Collider->SetRadius(0.5f);
  Collider->RegisterComponent();
}

APhysics3DKinematicActor::APhysics3DKinematicActor() {
  SetActorLocation3D({0.0f, 2.0f, 3.0f});
  ConfigureBody(*this, ERigidBody3DType::Kinematic);
  MBoxCollisionComponent3D* Collider = NewObject<MBoxCollisionComponent3D>(this);
  Collider->SetHalfExtent({0.5f, 0.5f, 0.5f});
  Collider->RegisterComponent();
}
