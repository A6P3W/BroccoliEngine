#include "World.h"

#include "ActorRegistry.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "ActorManager.h"
#include "Pawn.h"
#include "PlayerController.h"
#include "ReplicationSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "TimerManager.h"

World::World() {
  ActorManager = std::make_unique<FActorManager>();
  CollisionSystem = std::make_unique<FCollisionSystem>();
  SoundManager = std::make_unique<FSoundManager>();
  TimerManager = std::make_unique<FTimerManager>();
  ActorManager->SetWorld(this);
  ReplicationSystem = std::make_unique<FReplicationSystem>(this);
}

World::~World() { bTearingDown = true; }

void World::Update(float DeltaTime) {
  if (ActorManager) ActorManager->Update(DeltaTime);
  if (ReplicationSystem) ReplicationSystem->Update();
  if (ActorManager) {
    ActorManager->RemovePendingDestroy();
    ActorManager->FlushPendingActors();
  }
  if (bSimulating) {
    if (TimerManager) TimerManager->Update(DeltaTime);
    if (CollisionSystem) CollisionSystem->CheckCollisions();
  }
}

void World::Draw() {
  if (ActorManager) ActorManager->Draw();
}

void World::SetTargetFps(int NewTargetFps) {
  if (NewTargetFps <= 0) {
    return;
  }
  TargetFps = NewTargetFps;
}

void World::UpdateCurrentFps(float DeltaTime) {
  if (DeltaTime <= 0.0f) {
    return;
  }
  CurrentFps = 1.0f / DeltaTime;
  if (CurrentFps < TargetFps * 0.90f) {
    M_LOG("Warning: Current FPS ({}) is below 90% of target FPS ({}).", CurrentFps, TargetFps);
  }
}

void World::SetGameMode(AGameModeBase* mode) { GameMode = mode; }
ENetMode World::GetNetMode() const { return NetMode; }
void World::SetNetMode(ENetMode NewNetMode) { NetMode = NewNetMode; }
bool World::IsStandalone() const { return NetMode == ENetMode::Standalone; }
bool World::IsListenServer() const { return NetMode == ENetMode::ListenServer; }
bool World::IsClient() const { return NetMode == ENetMode::Client; }
bool World::IsServer() const { return IsStandalone() || IsListenServer(); }

FNetworkActorId World::AllocateNetworkActorId() {
  if (!IsServer()) {
    return 0;
  }
  return NextNetworkActorId++;
}

bool World::ServerTravel(FNetworkSceneId SceneId) {
  if (!IsServer() || !SceneManager::GetInstance().IsSceneRegistered(SceneId)) {
    return false;
  }
  if (IsListenServer() && ReplicationSystem) {
    ReplicationSystem->BroadcastServerTravel(SceneId);
  }
  return SceneManager::GetInstance().OpenLevelById(SceneId, GetNetMode());
}

bool World::ServerTravel(const std::string& LevelPath) {
  if (!IsServer() || LevelPath.empty()) {
    return false;
  }
  if (IsListenServer() && ReplicationSystem) {
    ReplicationSystem->BroadcastServerTravel(LevelPath);
  }
  return SceneManager::GetInstance().OpenLevelByPath(LevelPath, GetNetMode());
}

APlayerController* World::GetOrCreateLocalPlayerController() {
  if (LocalPlayerController && !LocalPlayerController->IsPendingDestroy()) {
    return LocalPlayerController;
  }
  AActor* controllerActor = nullptr;
  std::string controllerClass = LocalPlayerControllerClass;
  if (controllerClass.empty() && GameMode != nullptr) {
    controllerClass = GameMode->GetDefaultPlayerControllerClass();
  }
  if (!controllerClass.empty()) {
    controllerActor = ActorRegistry::GetInstance().Spawn(this, controllerClass);
  }
  if (!controllerActor) {
    controllerActor = SpawnActor<APlayerController>();
  }
  LocalPlayerController = dynamic_cast<APlayerController*>(controllerActor);
  if (!LocalPlayerController) {
    if (controllerActor) {
      controllerActor->Destroy();
    }
    LocalPlayerController = SpawnActor<APlayerController>();
  }
  LocalPlayerController->SetPlayerId(0);
  LocalPlayerController->bIsLocallyControlled = true;
  LocalPlayerController->SetupInputMappings();
  LocalPlayerController->Spawned();
  return LocalPlayerController;
}

void World::SetLocalPlayerController(APlayerController* PC) {
  LocalPlayerController = PC;
  if (LocalPlayerController) {
    LocalPlayerController->SetPlayerId(0);
    LocalPlayerController->bIsLocallyControlled = true;
    LocalPlayerController->SetupInputMappings();
  }
}

void World::PossessLocalPawn(APawn* Pawn) {
  if (!Pawn) {
    return;
  }
  if (LocalPlayerController) {
    LocalPlayerController->Possess(Pawn);
  }
}

void World::SetLocalPlayerControllerClass(const std::string& ClassName) {
  LocalPlayerControllerClass = ClassName;
}
