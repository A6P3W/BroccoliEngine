#include "World.h"

#include "ActorRegistry.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "ObjectManager.h"
#include "Pawn.h"
#include "PlayerController.h"
#include "ReplicationSystem.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "TimerManager.h"

World::World() {
  ObjectManager = std::make_unique<MObjectManager>();
  CollisionSystem = std::make_unique<MCollisionSystem>();
  SoundManager = std::make_unique<MSoundManager>();
  TimerManager = std::make_unique<MTimerManager>();
  ObjectManager->SetWorld(this);
  ReplicationSystem = std::make_unique<MReplicationSystem>(this);
}

World::~World() { bTrendingDown = true; }

void World::Update(float DeltaTime) {
  if (ObjectManager) ObjectManager->Update(DeltaTime);
  if (ReplicationSystem) ReplicationSystem->Update();
  if (ObjectManager) ObjectManager->RemovePendingDestroy();
  if (bSimulating) {
    if (TimerManager) TimerManager->Update(DeltaTime);
    if (CollisionSystem) CollisionSystem->CheckCollisions();
  }
}

void World::Draw() {
  if (ObjectManager) ObjectManager->Draw();
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
