#include "ActorManager.h"

#include <algorithm>

#include "Actor.h"

FActorManager::FActorManager() {}

FActorManager::~FActorManager() = default;

void FActorManager::Update(float DeltaTime) {
  for (auto& object : Actors) {
    if (object && !object->IsPendingDestroy()) {
      if (World->IsSimulating() || object->IsEditorActor() || object->CanUpdateAnytime()) {
        object->Update(DeltaTime);
      }
    }
  }
}

void FActorManager::Draw() {
  for (auto& object : Actors) {
    object->Draw();
  }
}

void FActorManager::RemovePendingDestroy() {
  auto shouldRemove = [](const std::unique_ptr<AActor>& obj) {
    return !obj || obj->IsPendingDestroy();
  };

  std::erase_if(Actors, shouldRemove);
  std::erase_if(PendingActors, shouldRemove);
}

void FActorManager::FlushPendingActors() {
  if (PendingActors.empty()) {
    return;
  }

  Actors.reserve(Actors.size() + PendingActors.size());
  for (auto& pendingObj : PendingActors) {
    Actors.push_back(std::move(pendingObj));
  }
  PendingActors.clear();
}

void FActorManager::ClearAllObjects() {
  Actors.clear();
  PendingActors.clear();
}
