#include "ActorManager.h"

#include <algorithm>
#include <vector>

#include "Actor.h"

struct FActorManager::Impl {
  std::vector<std::unique_ptr<AActor>> Actors;
  std::vector<std::unique_ptr<AActor>> PendingActors;
  World* World = nullptr;
};

FActorManager::FActorManager() : ImplPtr(new Impl()) {}

FActorManager::~FActorManager() { delete ImplPtr; }

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

void FActorManager::AddPendingActor(std::unique_ptr<AActor> Actor) {
  ImplPtr->PendingActors.push_back(std::move(Actor));
}

void FActorManager::RemovePendingDestroy() {
  auto shouldRemove = [](const std::unique_ptr<AActor>& obj) {
    return !obj || obj->IsPendingDestroy();
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

  std::erase_if(ImplPtr->Actors, shouldRemove);
  std::erase_if(ImplPtr->PendingActors, shouldRemove);
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
  ImplPtr->Actors.clear();
  ImplPtr->PendingActors.clear();
}
