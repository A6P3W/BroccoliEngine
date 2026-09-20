#include "CollisionComponent3D.h"

#include "Actor.h"
#include "PhysicsSystem3D.h"
#include "World.h"

void MCollisionComponent3D::OnRegister() {
  AActor* Owner = GetOwner();
  if (Owner && Owner->GetWorld() && Owner->GetWorld()->GetPhysicsSystem3D()) {
    Owner->GetWorld()->GetPhysicsSystem3D()->RefreshActorBody(Owner);
  }
}

void MCollisionComponent3D::OnUnregister() {
  AActor* Owner = GetOwner();
  if (Owner && Owner->GetWorld() && Owner->GetWorld()->GetPhysicsSystem3D()) {
    Owner->GetWorld()->GetPhysicsSystem3D()->UnregisterActorBody(Owner);
  }
}
