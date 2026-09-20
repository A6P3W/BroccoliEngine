#include "RigidBody3DComponent.h"

#include <algorithm>
#include <cmath>

#include "Actor.h"
#include "PhysicsSystem3D.h"
#include "SceneComponent.h"
#include "World.h"

namespace {
FPhysicsSystem3D* GetPhysicsSystem(const MRigidBody3DComponent& Component) {
  AActor* Owner = Component.GetOwner();
  return Owner && Owner->GetWorld() ? Owner->GetWorld()->GetPhysicsSystem3D() : nullptr;
}
}  // namespace

void MRigidBody3DComponent::SetBodyType(ERigidBody3DType NewBodyType) {
  if (BodyType == NewBodyType) {
    return;
  }
  BodyType = NewBodyType;
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->RefreshActorBody(GetOwner());
  }
}

void MRigidBody3DComponent::SetMass(float NewMass) {
  if (!std::isfinite(NewMass)) {
    return;
  }
  Mass = (std::max)(0.001f, NewMass);
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->RefreshActorBody(GetOwner());
  }
}

FVector3D MRigidBody3DComponent::GetLinearVelocity() const {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    return Physics->GetLinearVelocity(this);
  }
  return FVector3D::ZeroVector();
}

void MRigidBody3DComponent::SetLinearVelocity(const FVector3D& NewLinearVelocity) {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->SetLinearVelocity(this, NewLinearVelocity);
  }
}

void MRigidBody3DComponent::AddForce(const FVector3D& Force) {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->AddForce(this, Force);
  }
}

void MRigidBody3DComponent::AddImpulse(const FVector3D& Impulse) {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->AddImpulse(this, Impulse);
  }
}

void MRigidBody3DComponent::OnRegister() {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->RefreshActorBody(GetOwner());
  }
}

void MRigidBody3DComponent::OnUnregister() {
  if (FPhysicsSystem3D* Physics = GetPhysicsSystem(*this)) {
    Physics->UnregisterActorBody(GetOwner());
  }
}

void MRigidBody3DComponent::SyncTransformFromPhysics(
    const FVector3D& Location, const FQuaternion& Rotation
) {
  AActor* Owner = GetOwner();
  if (!Owner || !Owner->GetRootComponent()) {
    return;
  }
  Owner->GetRootComponent()->SetWorldLocation3D(Location);
  Owner->GetRootComponent()->SetWorldRotation3D(Rotation);
}
