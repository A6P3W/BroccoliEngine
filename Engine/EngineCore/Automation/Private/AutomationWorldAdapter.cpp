#include "AutomationWorldAdapter.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

#include "ActorManager.h"
#include "ActorRegistry.h"
#include "SceneManager.h"

namespace {
std::string GetCurrentSceneName(const SceneManager& Manager) {
  const std::string& LevelPath = Manager.GetCurrentLevelPath();
  return LevelPath.empty() ? std::string() : std::filesystem::path(LevelPath).stem().string();
}

EAutomationWorldReadStatus MakeActorSnapshot(
    AActor& Actor, World& CurrentWorld, FAutomationActorSnapshot& OutSnapshot
) {
  if (Actor.IsPendingDestroy() || Actor.GetWorld() != &CurrentWorld) {
    return EAutomationWorldReadStatus::ActorNotFound;
  }

  FAutomationActorSnapshot Snapshot;
  Snapshot.ActorId = Actor.GetActorId();
  Snapshot.InstanceName = Actor.GetInstanceName();
  Snapshot.ClassName = Actor.GetActorClassName();
  Snapshot.Location = Actor.GetActorLocation();
  Snapshot.Rotation = Actor.GetActorRotation();
  Snapshot.Scale = Actor.GetActorScale();
  if (Snapshot.ActorId == InvalidActorId || Snapshot.InstanceName.empty() ||
      Snapshot.ClassName.empty() || !std::isfinite(Snapshot.Location.X) ||
      !std::isfinite(Snapshot.Location.Y) || !std::isfinite(Snapshot.Rotation.Rotation) ||
      !std::isfinite(Snapshot.Scale.Scale)) {
    return EAutomationWorldReadStatus::InvalidState;
  }

  OutSnapshot = std::move(Snapshot);
  return EAutomationWorldReadStatus::Success;
}

EAutomationWorldReadStatus GetActorList(
    const FAutomationActorQuery& Query, FAutomationActorListSnapshot& OutSnapshot
) {
  SceneManager& Manager = SceneManager::GetInstance();
  World* CurrentWorld = Manager.GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldReadStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldReadStatus::InvalidState;
  }

  FAutomationActorListSnapshot Snapshot;
  Snapshot.SceneName = GetCurrentSceneName(Manager);
  for (const std::unique_ptr<AActor>& Actor : ActorManager->GetAllActors()) {
    if (!Actor || Actor->IsPendingDestroy()) {
      continue;
    }

    FAutomationActorSnapshot ActorSnapshot;
    if (MakeActorSnapshot(*Actor, *CurrentWorld, ActorSnapshot) !=
        EAutomationWorldReadStatus::Success) {
      return EAutomationWorldReadStatus::InvalidState;
    }
    if ((Query.ClassName && ActorSnapshot.ClassName != *Query.ClassName) ||
        (Query.InstanceName && ActorSnapshot.InstanceName != *Query.InstanceName)) {
      continue;
    }
    Snapshot.Actors.push_back(std::move(ActorSnapshot));
  }
  std::sort(
      Snapshot.Actors.begin(),
      Snapshot.Actors.end(),
      [](const FAutomationActorSnapshot& Left, const FAutomationActorSnapshot& Right) {
        return Left.ActorId < Right.ActorId;
      }
  );
  OutSnapshot = std::move(Snapshot);
  return EAutomationWorldReadStatus::Success;
}

EAutomationWorldReadStatus GetActor(FActorId ActorId, FAutomationActorSnapshot& OutSnapshot) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldReadStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldReadStatus::InvalidState;
  }

  AActor* Actor = ActorManager->FindActorById(ActorId);
  return Actor ? MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot)
               : EAutomationWorldReadStatus::ActorNotFound;
}

EAutomationWorldMutationStatus SpawnActor(
    const FAutomationSpawnActorRequest& Request, FAutomationActorSnapshot& OutSnapshot
) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldMutationStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldMutationStatus::InvalidState;
  }

  ActorRegistry& Registry = ActorRegistry::GetInstance();
  if (!Registry.Contains(Request.ClassName)) {
    return EAutomationWorldMutationStatus::ClassNotRegistered;
  }

  AActor* Actor =
      Registry.Spawn(CurrentWorld, Request.ClassName, Request.Location, Request.Rotation);
  if (!Actor || Actor->GetWorld() != CurrentWorld || Actor->HasBegunPlay()) {
    if (Actor) {
      Actor->Destroy();
    }
    return EAutomationWorldMutationStatus::InvalidState;
  }
  if (!Actor->SetActorScale(Request.Scale) ||
      (Request.InstanceName && !ActorManager->AssignInstanceName(*Actor, *Request.InstanceName))) {
    Actor->Destroy();
    return EAutomationWorldMutationStatus::InvalidState;
  }

  Actor->Spawned();
  return MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot) ==
                 EAutomationWorldReadStatus::Success
             ? EAutomationWorldMutationStatus::Success
             : EAutomationWorldMutationStatus::InvalidState;
}

EAutomationWorldMutationStatus DestroyActor(FActorId ActorId) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldMutationStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldMutationStatus::InvalidState;
  }

  AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
  if (!Actor) {
    return EAutomationWorldMutationStatus::ActorNotFound;
  }
  if (Actor->IsPendingDestroy()) {
    return EAutomationWorldMutationStatus::ActorPendingDestroy;
  }

  Actor->Destroy();
  return EAutomationWorldMutationStatus::Success;
}

EAutomationWorldMutationStatus PatchActorTransform(
    FActorId ActorId, const FAutomationTransformPatch& Patch, FAutomationActorSnapshot& OutSnapshot
) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldMutationStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldMutationStatus::InvalidState;
  }

  AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
  if (!Actor) {
    return EAutomationWorldMutationStatus::ActorNotFound;
  }
  if (Actor->IsPendingDestroy()) {
    return EAutomationWorldMutationStatus::ActorPendingDestroy;
  }
  if ((Patch.Location && !Actor->SetActorLocation(*Patch.Location)) ||
      (Patch.Rotation && !Actor->SetActorRotation(*Patch.Rotation)) ||
      (Patch.Scale && !Actor->SetActorScale(*Patch.Scale))) {
    return EAutomationWorldMutationStatus::InvalidState;
  }
  return MakeActorSnapshot(*Actor, *CurrentWorld, OutSnapshot) ==
                 EAutomationWorldReadStatus::Success
             ? EAutomationWorldMutationStatus::Success
             : EAutomationWorldMutationStatus::InvalidState;
}

EAutomationActorResolveStatus ResolveActor(FActorId ActorId, AActor*& OutActor) {
  OutActor = nullptr;
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationActorResolveStatus::WorldNotAvailable;
  }

  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationActorResolveStatus::InvalidState;
  }

  AActor* Actor = ActorManager->FindActorByIdIncludingPendingDestroy(ActorId);
  if (!Actor || Actor->GetWorld() != CurrentWorld) {
    return EAutomationActorResolveStatus::ActorNotFound;
  }
  if (Actor->IsPendingDestroy()) {
    return EAutomationActorResolveStatus::ActorPendingDestroy;
  }

  OutActor = Actor;
  return EAutomationActorResolveStatus::Success;
}
}  // namespace

FAutomationActorListProvider FAutomationWorldAdapter::CreateActorListProvider() {
  return GetActorList;
}

FAutomationActorProvider FAutomationWorldAdapter::CreateActorProvider() { return GetActor; }

FAutomationSpawnActorProvider FAutomationWorldAdapter::CreateSpawnActorProvider() {
  return SpawnActor;
}

FAutomationDestroyActorProvider FAutomationWorldAdapter::CreateDestroyActorProvider() {
  return DestroyActor;
}

FAutomationPatchActorTransformProvider FAutomationWorldAdapter::CreateTransformProvider() {
  return PatchActorTransform;
}

FAutomationActorResolver FAutomationWorldAdapter::CreateActorResolver() { return ResolveActor; }
