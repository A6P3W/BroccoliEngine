#include "Collision3DComponent.h"

#include <algorithm>

#include "Actor.h"
#include "PhysicsSystem3D.h"
#include "World.h"

void MCollision3DComponent::OnRegister() {
  AActor* Owner = GetOwner();
  if (Owner && Owner->GetWorld() && Owner->GetWorld()->GetPhysicsSystem3D()) {
    Owner->GetWorld()->GetPhysicsSystem3D()->RefreshActorBody(Owner);
  }
}

void MCollision3DComponent::OnUnregister() {
  AActor* Owner = GetOwner();
  if (Owner && Owner->GetWorld() && Owner->GetWorld()->GetPhysicsSystem3D()) {
    Owner->GetWorld()->GetPhysicsSystem3D()->UnregisterActorBody(Owner);
  }
}

bool MCollision3DComponent::IsOverlappingActor(AActor* OtherActor) const {
  return std::find(OverlappingActors.begin(), OverlappingActors.end(), OtherActor) !=
         OverlappingActors.end();
}

std::vector<AActor*> MCollision3DComponent::GetOverlappingActors() const {
  std::vector<AActor*> Result;
  Result.reserve(OverlappingActors.size());
  for (AActor* Actor : OverlappingActors) {
    if (Actor && Actor != GetOwner() && !Actor->IsPendingDestroy()) {
      Result.push_back(Actor);
    }
  }
  return Result;
}

void MCollision3DComponent::NotifyOverlapBegin(AActor* OtherActor) {
  AActor* Owner = GetOwner();
  if (!Owner || !OtherActor || Owner == OtherActor || OtherActor->IsPendingDestroy()) {
    return;
  }
  if (!IsOverlappingActor(OtherActor)) {
    OverlappingActors.push_back(OtherActor);
    Owner->BeginOverlap(OtherActor);
  }
}

void MCollision3DComponent::NotifyOverlapEnd(AActor* OtherActor) {
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

void MCollision3DComponent::RemoveOverlappingActor(AActor* OtherActor) {
  std::erase(OverlappingActors, OtherActor);
}
