#include "ActorManager.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
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

std::string RemoveNumericSuffix(const std::string& Name) {
  const size_t Separator = Name.rfind('_');
  if (Separator == std::string::npos || Separator == 0 || Separator + 1 >= Name.size()) {
    return Name;
  }

  for (size_t Index = Separator + 1; Index < Name.size(); ++Index) {
    if (!std::isdigit(static_cast<unsigned char>(Name[Index]))) {
      return Name;
    }
  }

  return Name.substr(0, Separator);
}
}  // namespace

struct FActorManager::Impl {
  std::vector<std::unique_ptr<AActor>> Actors;
  std::vector<std::unique_ptr<AActor>> PendingActors;
  std::unordered_map<FActorId, AActor*> ActorIdMap;
  std::unordered_map<std::string, AActor*> InstanceNames;
  std::unordered_map<std::string, uint64_t> NextInstanceIndex;
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

size_t FActorManager::GetActiveActorCount() const {
  return static_cast<size_t>(std::count_if(
      ImplPtr->Actors.begin(), ImplPtr->Actors.end(), [](const std::unique_ptr<AActor>& Actor) {
        return Actor && !Actor->IsPendingDestroy();
      }
  ));
}

AActor* FActorManager::FindActorById(FActorId ActorId) {
  return const_cast<AActor*>(static_cast<const FActorManager*>(this)->FindActorById(ActorId));
}

const AActor* FActorManager::FindActorById(FActorId ActorId) const {
  const AActor* Actor = FindActorByIdIncludingPendingDestroy(ActorId);
  return Actor && !Actor->IsPendingDestroy() ? Actor : nullptr;
}

AActor* FActorManager::FindActorByIdIncludingPendingDestroy(FActorId ActorId) {
  return const_cast<AActor*>(
      static_cast<const FActorManager*>(this)->FindActorByIdIncludingPendingDestroy(ActorId)
  );
}

const AActor* FActorManager::FindActorByIdIncludingPendingDestroy(FActorId ActorId) const {
  if (ActorId == InvalidActorId || !ImplPtr->World) {
    return nullptr;
  }

  const auto Iterator = ImplPtr->ActorIdMap.find(ActorId);
  if (Iterator == ImplPtr->ActorIdMap.end()) {
    return nullptr;
  }

  AActor* Actor = Iterator->second;
  if (!Actor || Actor->GetActorId() != ActorId || Actor->GetWorld() != ImplPtr->World) {
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

std::string FActorManager::AllocateUniqueInstanceName(
    const std::string& RequestedName, const std::string& ClassName
) {
  if (!RequestedName.empty() && !ImplPtr->InstanceNames.contains(RequestedName)) {
    return RequestedName;
  }

  std::string BaseName = RequestedName.empty() ? ClassName : RemoveNumericSuffix(RequestedName);
  if (BaseName.empty()) {
    BaseName = "Actor";
  }

  uint64_t& NextIndex = ImplPtr->NextInstanceIndex[BaseName];
  if (NextIndex == 0) {
    NextIndex = 1;
  }

  std::string Candidate;
  do {
    Candidate = BaseName + "_" + std::to_string(NextIndex++);
  } while (ImplPtr->InstanceNames.contains(Candidate));

  return Candidate;
}

void FActorManager::RegisterInstanceName(AActor& Actor) { AssignInstanceName(Actor, ""); }

bool FActorManager::AssignInstanceName(AActor& Actor, const std::string& RequestedName) {
  if (!ImplPtr->World || Actor.GetWorld() != ImplPtr->World) {
    return false;
  }

  const std::string PreviousName = Actor.GetInstanceName();
  if (!PreviousName.empty()) {
    const auto PreviousNameIt = ImplPtr->InstanceNames.find(PreviousName);
    if (PreviousNameIt != ImplPtr->InstanceNames.end() && PreviousNameIt->second == &Actor) {
      ImplPtr->InstanceNames.erase(PreviousNameIt);
    }
  }

  std::string NewName = AllocateUniqueInstanceName(RequestedName, Actor.GetActorClassName());
  ImplPtr->InstanceNames.emplace(NewName, &Actor);
  Actor.SetInstanceNameInternal(std::move(NewName));
  return true;
}

void FActorManager::UnregisterInstanceName(AActor& Actor) {
  const std::string& InstanceName = Actor.GetInstanceName();
  if (!InstanceName.empty()) {
    const auto InstanceNameIt = ImplPtr->InstanceNames.find(InstanceName);
    if (InstanceNameIt != ImplPtr->InstanceNames.end() && InstanceNameIt->second == &Actor) {
      ImplPtr->InstanceNames.erase(InstanceNameIt);
    }
  }

  Actor.InvalidateInstanceNameInternal();
}

void FActorManager::RemovePendingDestroy() {
  auto ShouldRemove = [this](const std::unique_ptr<AActor>& Actor) {
    if (Actor && Actor->IsPendingDestroy()) {
      UnregisterInstanceName(*Actor);
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
      Actor->InvalidateInstanceNameInternal();
    }
  }
  for (auto& Actor : ImplPtr->PendingActors) {
    if (Actor) {
      Actor->InvalidateActorIdInternal();
      Actor->InvalidateInstanceNameInternal();
    }
  }
  ImplPtr->ActorIdMap.clear();
  ImplPtr->InstanceNames.clear();
  ImplPtr->NextInstanceIndex.clear();
  ImplPtr->Actors.clear();
  ImplPtr->PendingActors.clear();
}
