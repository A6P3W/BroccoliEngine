#include "WorldService.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

#include "ActorManager.h"
#include "ActorRegistry.h"
#include "PhysicsSystem3D.h"
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
  Snapshot.Location = Actor.GetActorLocation3D();
  Snapshot.Rotation = Actor.GetActorRotation3D().ToRotator();
  Snapshot.Scale = Actor.GetActorScale3D();
  if (Snapshot.ActorId == InvalidActorId || Snapshot.InstanceName.empty() ||
      Snapshot.ClassName.empty() || !std::isfinite(Snapshot.Location.X) ||
      !std::isfinite(Snapshot.Location.Y) || !std::isfinite(Snapshot.Location.Z) ||
      !std::isfinite(Snapshot.Rotation.Pitch) || !std::isfinite(Snapshot.Rotation.Yaw) ||
      !std::isfinite(Snapshot.Rotation.Roll) || !std::isfinite(Snapshot.Scale.X) ||
      !std::isfinite(Snapshot.Scale.Y) || !std::isfinite(Snapshot.Scale.Z)) {
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

EAutomationWorldReadStatus GetActorComponents(
    FActorId ActorId, FAutomationActorComponentListSnapshot& OutSnapshot
) {
  World* CurrentWorld = SceneManager::GetInstance().GetCurrentScene();
  if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
    return EAutomationWorldReadStatus::WorldNotAvailable;
  }
  FActorManager* ActorManager = CurrentWorld->GetActorManager();
  if (!ActorManager || ActorManager->GetWorld() != CurrentWorld) {
    return EAutomationWorldReadStatus::InvalidState;
  }
  AActor* Actor = ActorManager->FindActorById(ActorId);
  if (!Actor || Actor->IsPendingDestroy() || Actor->GetWorld() != CurrentWorld) {
    return EAutomationWorldReadStatus::ActorNotFound;
  }

  FAutomationActorComponentListSnapshot Snapshot;
  Snapshot.ActorId = ActorId;
  Snapshot.ClassName = Actor->GetActorClassName();
  const auto& Components = Actor->GetComponents();
  Snapshot.Components.reserve(Components.size());
  for (size_t Index = 0; Index < Components.size(); ++Index) {
    const std::unique_ptr<MActorComponent>& Component = Components[Index];
    if (!Component) {
      continue;
    }
    Snapshot.Components.push_back(
        {Component->GetComponentId(),
         Component->GetNetComponentName(),
         Component->GetComponentClassName(),
         Component->IsRegistered(),
         Component->IsPendingDestroy(),
         Component->bReplicates,
         Component->ComponentNetworkId}
    );
  }
  OutSnapshot = std::move(Snapshot);
  return EAutomationWorldReadStatus::Success;
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

  AActor* Actor = Registry.Spawn(
      CurrentWorld,
      Request.ClassName,
      {Request.Location.X, Request.Location.Y},
      FRotator(Request.Rotation.Roll)
  );
  if (!Actor || Actor->GetWorld() != CurrentWorld || Actor->HasBegunPlay()) {
    if (Actor) {
      Actor->Destroy();
    }
    return EAutomationWorldMutationStatus::InvalidState;
  }
  if (!Actor->SetActorLocation3D(Request.Location) ||
      !Actor->SetActorRotation3D(FQuaternion::FromRotator(Request.Rotation)) ||
      !Actor->SetActorScale3D(Request.Scale) ||
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

  if (Patch.Location.HasAnyValue()) {
    FVector3D NewLocation = Actor->GetActorLocation3D();
    if (Patch.Location.X) NewLocation.X = *Patch.Location.X;
    if (Patch.Location.Y) NewLocation.Y = *Patch.Location.Y;
    if (Patch.Location.Z) NewLocation.Z = *Patch.Location.Z;
    if (!Actor->SetActorLocation3D(NewLocation)) {
      return EAutomationWorldMutationStatus::InvalidState;
    }
  }

  if (Patch.Rotation.HasAnyValue()) {
    FRotator3D NewRotation = Actor->GetActorRotation3D().ToRotator();
    if (Patch.Rotation.Pitch) NewRotation.Pitch = *Patch.Rotation.Pitch;
    if (Patch.Rotation.Yaw) NewRotation.Yaw = *Patch.Rotation.Yaw;
    if (Patch.Rotation.Roll) NewRotation.Roll = *Patch.Rotation.Roll;
    if (!Actor->SetActorRotation3D(FQuaternion::FromRotator(NewRotation))) {
      return EAutomationWorldMutationStatus::InvalidState;
    }
  }

  if (Patch.Scale.HasAnyValue()) {
    FScale3D NewScale = Actor->GetActorScale3D();
    if (Patch.Scale.X) NewScale.X = *Patch.Scale.X;
    if (Patch.Scale.Y) NewScale.Y = *Patch.Scale.Y;
    if (Patch.Scale.Z) NewScale.Z = *Patch.Scale.Z;
    if (!Actor->SetActorScale3D(NewScale)) {
      return EAutomationWorldMutationStatus::InvalidState;
    }
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

EAutomationComponentResolveStatus ResolveComponent(
    FActorId ActorId, FComponentId ComponentId, MActorComponent*& OutComponent
) {
  OutComponent = nullptr;
  AActor* Actor = nullptr;
  const EAutomationActorResolveStatus ActorStatus = ResolveActor(ActorId, Actor);
  if (ActorStatus != EAutomationActorResolveStatus::Success) {
    switch (ActorStatus) {
      case EAutomationActorResolveStatus::WorldNotAvailable:
        return EAutomationComponentResolveStatus::WorldNotAvailable;
      case EAutomationActorResolveStatus::ActorNotFound:
        return EAutomationComponentResolveStatus::ActorNotFound;
      case EAutomationActorResolveStatus::ActorPendingDestroy:
        return EAutomationComponentResolveStatus::ActorPendingDestroy;
      case EAutomationActorResolveStatus::InvalidState:
      case EAutomationActorResolveStatus::Success:
        return EAutomationComponentResolveStatus::InvalidState;
    }
    return EAutomationComponentResolveStatus::InvalidState;
  }
  MActorComponent* Component = Actor->FindComponentById(ComponentId);
  if (!Component) {
    return EAutomationComponentResolveStatus::ComponentNotFound;
  }
  if (Component->IsPendingDestroy()) {
    return EAutomationComponentResolveStatus::ComponentPendingDestroy;
  }
  OutComponent = Component;
  return EAutomationComponentResolveStatus::Success;
}
}  // namespace

FAutomationActorListProvider FAutomationWorldService::CreateActorListProvider() {
  return GetActorList;
}

FAutomationWorldStateProvider FAutomationWorldService::CreateWorldStateProvider() {
  return [] {
    FAutomationWorldStateSnapshot Snapshot;
    SceneManager& Manager = SceneManager::GetInstance();
    World* CurrentWorld = Manager.GetCurrentScene();
    if (!CurrentWorld || CurrentWorld->IsTearingDown()) {
      return Snapshot;
    }

    Snapshot.SceneName = GetCurrentSceneName(Manager);
    Snapshot.Fps = CurrentWorld->GetCurrentFps();
    Snapshot.Simulating = CurrentWorld->IsSimulating();
    Snapshot.WorldAvailable = true;
    if (const FActorManager* ActorManager = CurrentWorld->GetActorManager()) {
      Snapshot.ActorCount = static_cast<uint32_t>(ActorManager->GetActiveActorCount());
    }
    if (const FPhysicsSystem3D* PhysicsSystem = CurrentWorld->GetPhysicsSystem3D()) {
      Snapshot.Physics3DAvailable = PhysicsSystem->IsInitialized();
      Snapshot.Physics3DBodyCount = PhysicsSystem->GetBodyCount();
    }
    return Snapshot;
  };
}

FAutomationActorProvider FAutomationWorldService::CreateActorProvider() { return GetActor; }

FAutomationActorComponentListProvider FAutomationWorldService::CreateActorComponentListProvider() {
  return GetActorComponents;
}

FAutomationSpawnActorProvider FAutomationWorldService::CreateSpawnActorProvider() {
  return SpawnActor;
}

FAutomationDestroyActorProvider FAutomationWorldService::CreateDestroyActorProvider() {
  return DestroyActor;
}

FAutomationPatchActorTransformProvider FAutomationWorldService::CreateTransformProvider() {
  return PatchActorTransform;
}

FAutomationActorResolver FAutomationWorldService::CreateActorResolver() { return ResolveActor; }

FAutomationComponentResolver FAutomationWorldService::CreateComponentResolver() {
  return ResolveComponent;
}
