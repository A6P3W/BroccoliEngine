#include "ActorManager.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Actor.h"
namespace {
std::atomic<FActorId> NextActorId = 1;

FActorId AllocateActorId() {
  FActorId Candidate = NextActorId.load(std::memory_order_relaxed);
  while (Candidate != InvalidActorId) {
    const FActorId NextCandidate =
        Candidate == (std::numeric_limits<FActorId>::max)() ? InvalidActorId : Candidate + 1;
    if (NextActorId.compare_exchange_weak(
            Candidate, NextCandidate, std::memory_order_relaxed, std::memory_order_relaxed
        )) {
      return Candidate;
    }
  }
  throw std::overflow_error("ActorId allocator exhausted");
}
}  // namespace

struct FActorManager::Impl {
  std::vector<std::unique_ptr<AActor>> Actors;
  std::vector<std::unique_ptr<AActor>> PendingActors;
  std::unordered_map<FActorId, AActor*> ActorIdMap;
  World* World = nullptr;
};

FActorManager::FActorManager() : ImplPtr(new Impl()) {}

FActorManager::~FActorManager() {
  ClearAllObjects();
  delete ImplPtr;
}

void FActorManager::Update(float DeltaTime) {
  for (auto& object : ImplPtr->Actors) {
    if (object && !object->IsPendingDestroy()) {
      if (ImplPtr->World->IsSimulating() || object->IsEditorActor() || object->CanUpdateAnytime()) {
        object->Update(DeltaTime);
      }
    }
  }
}

void FActorManager::Draw() {
  for (auto& object : ImplPtr->Actors) {
    object->Draw();
  }
}

void FActorManager::SetWorld(World* world) { ImplPtr->World = world; }
World* FActorManager::GetWorld() const { return ImplPtr->World; }

const std::vector<std::unique_ptr<AActor>>& FActorManager::GetAllActors() const {
  return ImplPtr->Actors;
}

AActor* FActorManager::FindActorById(FActorId ActorId) {
  return const_cast<AActor*>(static_cast<const FActorManager*>(this)->FindActorById(ActorId));
}

const AActor* FActorManager::FindActorById(FActorId ActorId) const {
  if (ActorId == InvalidActorId || !ImplPtr->World) {
    return nullptr;
  }

  const auto Iterator = ImplPtr->ActorIdMap.find(ActorId);
  if (Iterator == ImplPtr->ActorIdMap.end()) {
    return nullptr;
  }

  AActor* Actor = Iterator->second;
  if (!Actor || Actor->IsPendingDestroy() || Actor->GetActorId() != ActorId ||
      Actor->GetWorld() != ImplPtr->World) {
    return nullptr;
  }
  return Actor;
}

void FActorManager::AddPendingActor(std::unique_ptr<AActor> Actor) {
  ImplPtr->PendingActors.push_back(std::move(Actor));
}

void FActorManager::RegisterActorId(AActor& Actor) {
  const FActorId ActorId = AllocateActorId();
  Actor.SetActorIdInternal(ActorId);
  ImplPtr->ActorIdMap.emplace(ActorId, &Actor);
}

void FActorManager::UnregisterActorId(AActor& Actor) {
  const FActorId ActorId = Actor.GetActorId();
  if (ActorId != InvalidActorId) {
    const auto Iterator = ImplPtr->ActorIdMap.find(ActorId);
    if (Iterator != ImplPtr->ActorIdMap.end() && Iterator->second == &Actor) {
      ImplPtr->ActorIdMap.erase(Iterator);
    }
  }
  Actor.InvalidateActorIdInternal();
}

void FActorManager::RemovePendingDestroy() {
  auto ShouldRemove = [this](const std::unique_ptr<AActor>& Actor) {
    if (Actor && Actor->IsPendingDestroy()) {
      UnregisterActorId(*Actor);
      return true;
    }
    return !Actor;
  };

  if (ImplPtr->World && ImplPtr->World->GetCollisionSystem()) {
    for (auto& obj : ImplPtr->Actors) {
      if (obj && obj->IsPendingDestroy()) {
        ImplPtr->World->GetCollisionSystem()->RemoveActorReferences(obj.get());
      }
    }
    for (auto& obj : ImplPtr->PendingActors) {
      if (obj && obj->IsPendingDestroy()) {
        ImplPtr->World->GetCollisionSystem()->RemoveActorReferences(obj.get());
      }
    }
  }

  std::erase_if(ImplPtr->Actors, ShouldRemove);
  std::erase_if(ImplPtr->PendingActors, ShouldRemove);
}

void FActorManager::FlushPendingActors() {
  if (ImplPtr->PendingActors.empty()) {
    return;
  }

  ImplPtr->Actors.reserve(ImplPtr->Actors.size() + ImplPtr->PendingActors.size());
  for (auto& pendingObj : ImplPtr->PendingActors) {
    ImplPtr->Actors.push_back(std::move(pendingObj));
  }
  ImplPtr->PendingActors.clear();
}

void FActorManager::ClearAllObjects() {
  for (auto& Actor : ImplPtr->Actors) {
    if (Actor) {
      Actor->InvalidateActorIdInternal();
    }
  }
  for (auto& Actor : ImplPtr->PendingActors) {
    if (Actor) {
      Actor->InvalidateActorIdInternal();
    }
  }
  ImplPtr->ActorIdMap.clear();
  ImplPtr->Actors.clear();
  ImplPtr->PendingActors.clear();
}
