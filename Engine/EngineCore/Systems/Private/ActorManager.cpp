#include "ActorManager.h"

#include <algorithm>

#include "Actor.h"

FActorManager::FActorManager() {}

FActorManager::~FActorManager() = default;

void FActorManager::Update(float DeltaTime) {
  // 更新対象のポインタをコピーしてループを回す
  std::vector<AActor*> tempActors;
  tempActors.reserve(Actors.size());
  for (auto& object : Actors) {
    tempActors.push_back(object.get());
  }
  for (auto* object : tempActors) {
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
  std::erase_if(Actors, [](const std::unique_ptr<AActor>& obj) {
    return !obj || obj->IsPendingDestroy();
  });
}

void FActorManager::ClearAllObjects() { Actors.clear(); }
