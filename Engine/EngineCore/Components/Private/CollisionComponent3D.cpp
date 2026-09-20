#include "CollisionComponent3D.h"

#include <algorithm>

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

bool MCollisionComponent3D::IsOverlappingActor(AActor* OtherActor) const {
  return std::find(OverlappingActors.begin(), OverlappingActors.end(), OtherActor) !=
         OverlappingActors.end();
}

std::vector<AActor*> MCollisionComponent3D::GetOverlappingActors() const {
  std::vector<AActor*> Result;
  Result.reserve(OverlappingActors.size());
  for (AActor* Actor : OverlappingActors) {
    if (Actor && Actor != GetOwner() && !Actor->IsPendingDestroy()) {
      Result.push_back(Actor);
    }
  }
  return Result;
}

void MCollisionComponent3D::NotifyOverlapBegin(AActor* OtherActor) {
  AActor* Owner = GetOwner();
  if (!Owner || !OtherActor || Owner == OtherActor || OtherActor->IsPendingDestroy()) {
    return;
  }
  if (!IsOverlappingActor(OtherActor)) {
    OverlappingActors.push_back(OtherActor);
    Owner->BeginOverlap(OtherActor);
  }
}

void MCollisionComponent3D::NotifyOverlapEnd(AActor* OtherActor) {
  AActor* Owner = GetOwner();
  const auto It = std::find(OverlappingActors.begin(), OverlappingActors.end(), OtherActor);
  if (It == OverlappingActors.end()) {
    return;
  }
  OverlappingActors.erase(It);
  if (Owner && OtherActor && !Owner->IsPendingDestroy() && !OtherActor->IsPendingDestroy()) {
    Owner->EndOverlap(OtherActor);
  }
}

void MCollisionComponent3D::RemoveOverlappingActor(AActor* OtherActor) {
  std::erase(OverlappingActors, OtherActor);
}
