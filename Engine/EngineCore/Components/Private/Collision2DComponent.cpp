#include "Collision2DComponent.h"

#include <CollisionSystem.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Actor.h"
#include "World.h"

struct MCollision2DComponent::Impl {
  std::unordered_set<AActor*> CheckedThisFrame;
  std::unordered_set<AActor*> OverlappingActors;
  std::unordered_map<MCollision2DComponent*, std::uint64_t> LastCheckedFrame;
  std::unordered_set<AActor*> IntersectingThisFrame;
  ECollisionType CollisionType = ECollisionType::Block;
  bool bIsStatic = true;
};

MCollision2DComponent::MCollision2DComponent() : ImplPtr(new Impl()) {}

MCollision2DComponent::~MCollision2DComponent() {
  if (!GetOwner()->GetWorld()->IsTearingDown()) {
    UnRegisterComponent();
  }
  delete ImplPtr;
}

ECollisionType MCollision2DComponent::GetCollisionType() const { return ImplPtr->CollisionType; }

void MCollision2DComponent::SetCollisionType(ECollisionType NewType) {
  ImplPtr->CollisionType = NewType;
}

bool MCollision2DComponent::IsStatic() const { return ImplPtr->bIsStatic; }

void MCollision2DComponent::MarkCheckedThisFrame(AActor* OtherActor) {
  ImplPtr->CheckedThisFrame.insert(OtherActor);
}

void MCollision2DComponent::FlushOverlapState() {
  std::vector<AActor*> toRemove;
  for (AActor* actor : ImplPtr->OverlappingActors) {
    if (ImplPtr->IntersectingThisFrame.find(actor) == ImplPtr->IntersectingThisFrame.end()) {
      toRemove.push_back(actor);
    }
  }
  for (AActor* actor : toRemove) {
    ImplPtr->OverlappingActors.erase(actor);
    GetOwner()->EndOverlap(actor);
  }
  ImplPtr->CheckedThisFrame.clear();
  ImplPtr->IntersectingThisFrame.clear();
}
void MCollision2DComponent::OnRegister() {
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
    GetOwner()->GetWorld()->GetCollisionSystem()->RegisterCollision(this);
  }
}

void MCollision2DComponent::OnUnregister() {
  if (GetOwner() && GetOwner()->GetWorld()) {
    if (auto CS = GetOwner()->GetWorld()->GetCollisionSystem()) {
      CS->UnRegisterCollision(this);
    }
  }
}
void MCollision2DComponent::OnComponentDestroy() {}

void MCollision2DComponent::SetStatic(bool IsStatic) {
  if (ImplPtr->bIsStatic == IsStatic) {
    return;
  }
  ImplPtr->bIsStatic = IsStatic;
  if (GetOwner() && GetOwner()->GetWorld() && GetOwner()->GetWorld()->GetCollisionSystem()) {
    GetOwner()->GetWorld()->GetCollisionSystem()->RebuildStaticCollisionMap();
  }
}

bool MCollision2DComponent::IsOverlappingActor(AActor* OtherActor) const {
  return ImplPtr->OverlappingActors.contains(OtherActor);
}

std::vector<AActor*> MCollision2DComponent::GetOverlappingActors() const {
  std::vector<AActor*> Result;
  Result.reserve(ImplPtr->OverlappingActors.size());

  for (AActor* Actor : ImplPtr->OverlappingActors) {
    if (!Actor || Actor == GetOwner()) {
      continue;
    }
    Result.push_back(Actor);
  }

  return Result;
}

bool MCollision2DComponent::ShouldProcessPair(
    MCollision2DComponent* OtherComponent, std::uint64_t FrameId
) {
  auto it = ImplPtr->LastCheckedFrame.find(OtherComponent);
  if (it != ImplPtr->LastCheckedFrame.end() && it->second == FrameId) {
    ImplPtr->CheckedThisFrame.insert(OtherComponent->GetOwner());
    return false;
  }

  ImplPtr->LastCheckedFrame[OtherComponent] = FrameId;

  auto& otherFrame = OtherComponent->ImplPtr->LastCheckedFrame;
  otherFrame[this] = FrameId;

  return true;
}

void MCollision2DComponent::UpdateOverlapState(AActor* OtherActor, bool bIsIntersecting) {
  ImplPtr->CheckedThisFrame.insert(OtherActor);
  if (bIsIntersecting) {
    ImplPtr->IntersectingThisFrame.insert(OtherActor);
    if (ImplPtr->OverlappingActors.insert(OtherActor).second) {
      GetOwner()->BeginOverlap(OtherActor);
    }
  }
}

void MCollision2DComponent::RemoveActorReference(AActor* Actor) {
  if (!Actor) return;
  ImplPtr->OverlappingActors.erase(Actor);
  ImplPtr->CheckedThisFrame.erase(Actor);
  ImplPtr->IntersectingThisFrame.erase(Actor);
}
