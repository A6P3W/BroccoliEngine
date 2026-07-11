#include "CollisionComponent.h"

#include <CollisionSystem.h>

#include "Actor.h"
#include "World.h"

MCollisionComponent::MCollisionComponent() {}

MCollisionComponent::~MCollisionComponent() {
  if (!GetOwner()->GetWorld()->IsTearingDown()) {
    UnRegisterComponent();
  }
}
void MCollisionComponent::MarkCheckedThisFrame(AActor* OtherActor) {
  CheckedThisFrame.insert(OtherActor);
}

void MCollisionComponent::FlushOverlapState() {
  std::vector<AActor*> toRemove;
  for (AActor* actor : OverlappingActors) {
    if (IntersectingThisFrame.find(actor) == IntersectingThisFrame.end()) {
      toRemove.push_back(actor);
    }
  }
  for (AActor* actor : toRemove) {
    OverlappingActors.erase(actor);
    GetOwner()->EndOverlap(actor);
  }
  CheckedThisFrame.clear();
  IntersectingThisFrame.clear();
}
void MCollisionComponent::OnRegister() {
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
    GetOwner()->GetWorld()->GetCollisionSystem()->RegisterCollision(this);
  }
}

void MCollisionComponent::OnUnregister() {
  if (GetOwner() && GetOwner()->GetWorld()) {
    if (auto CS = GetOwner()->GetWorld()->GetCollisionSystem()) {
      CS->UnRegisterCollision(this);
    }
  }
}
void MCollisionComponent::OnComponentDestroy() {}

void MCollisionComponent::SetStatic(bool IsStatic) {
  if (bIsStatic == IsStatic) {
    return;
  }
  bIsStatic = IsStatic;
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
    GetOwner()->GetWorld()->GetCollisionSystem()->RebuildStaticCollisionMap();
  }
}

bool MCollisionComponent::IsOverlappingActor(AActor* OtherActor) const {
  return OverlappingActors.contains(OtherActor);
}

bool MCollisionComponent::ShouldProcessPair(MCollisionComponent* OtherComponent, std::uint64_t FrameId) {
  auto it = LastCheckedFrame.find(OtherComponent);
  if (it != LastCheckedFrame.end() && it->second == FrameId) {
    CheckedThisFrame.insert(OtherComponent->GetOwner());
    return false;
  }

  LastCheckedFrame[OtherComponent] = FrameId;

  auto& otherFrame = OtherComponent->LastCheckedFrame;
  otherFrame[this] = FrameId;

  return true;
}

void MCollisionComponent::UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting) {
  CheckedThisFrame.insert(OtherActor);
  if (bIsIntersecting) {
    IntersectingThisFrame.insert(OtherActor);
    if (OverlappingActors.insert(OtherActor).second) {
      GetOwner()->BeginOverlap(OtherActor);
    }
  }
}
