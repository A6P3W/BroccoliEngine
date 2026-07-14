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
#include "DebugOverlay.h"
struct World::Impl {
  std::unique_ptr<FCollisionSystem> CollisionSystem;
  std::unique_ptr<FSoundManager> SoundManager;
  std::unique_ptr<FTimerManager> TimerManager;
  std::unique_ptr<FReplicationSystem> ReplicationSystem;
  std::unique_ptr<FActorManager> ActorManager;
  AGameModeBase* GameMode = nullptr;
  bool Simulating = true;
  bool TearingDown = false;
  int TargetFps = 60;
  float CurrentFps = 0.0f;
  ENetMode NetMode = ENetMode::Standalone;
  FNetworkActorId NextNetworkActorId = 1;
  APlayerController* LocalPlayerController = nullptr;
  std::string LocalPlayerControllerClass;
};
World::World() : ImplPtr(new Impl()) {
  ImplPtr->ActorManager = std::make_unique<FActorManager>();
  ImplPtr->CollisionSystem = std::make_unique<FCollisionSystem>();
  ImplPtr->SoundManager = std::make_unique<FSoundManager>();
  ImplPtr->TimerManager = std::make_unique<FTimerManager>();
  ImplPtr->ActorManager->SetWorld(this);
  ImplPtr->ReplicationSystem = std::make_unique<FReplicationSystem>(this);
}

World::~World() {
  ImplPtr->TearingDown = true;
  delete ImplPtr;
}

FActorManager* World::GetActorManager() { return ImplPtr->ActorManager.get(); }

FActorManager* World::GetObjectManager() { return ImplPtr->ActorManager.get(); }
FCollisionSystem* World::GetCollisionSystem() { return ImplPtr->CollisionSystem.get(); }
FSoundManager* World::GetSoundManager() { return ImplPtr->SoundManager.get(); }
FTimerManager* World::GetTimerManager() { return ImplPtr->TimerManager.get(); }
FReplicationSystem* World::GetReplicationSystem() { return ImplPtr->ReplicationSystem.get(); }
AGameModeBase* World::GetGameMode() const { return ImplPtr->GameMode; }
void World::SetSimulating(bool bRunning) { ImplPtr->Simulating = bRunning; }
bool World::IsSimulating() const { return ImplPtr->Simulating; }
bool World::IsTearingDown() const { return ImplPtr->TearingDown; }
int World::GetTargetFps() const { return ImplPtr->TargetFps; }
float World::GetCurrentFps() const { return ImplPtr->CurrentFps; }
void World::Update(float DeltaTime) {
  if (ImplPtr->ActorManager) ImplPtr->ActorManager->Update(DeltaTime);
  if (ImplPtr->ReplicationSystem) ImplPtr->ReplicationSystem->Update();
  if (ImplPtr->ActorManager) {
    ImplPtr->ActorManager->RemovePendingDestroy();
    ImplPtr->ActorManager->FlushPendingActors();
  }
  if (ImplPtr->Simulating) {
    if (ImplPtr->TimerManager) ImplPtr->TimerManager->Update(DeltaTime);
    if (ImplPtr->CollisionSystem) ImplPtr->CollisionSystem->CheckCollisions();
  }
}

void World::Draw() {
  if (ImplPtr->ActorManager) ImplPtr->ActorManager->Draw();
}

void World::SetTargetFps(int NewTargetFps) {
  if (NewTargetFps <= 0) {
    return;
  }
  ImplPtr->TargetFps = NewTargetFps;
}

void World::UpdateCurrentFps(float DeltaTime) {
  if (DeltaTime <= 0.0f) {
    return;
  }
  ImplPtr->CurrentFps = 1.0f / DeltaTime;
}

void World::SetGameMode(AGameModeBase* mode) { ImplPtr->GameMode = mode; }
ENetMode World::GetNetMode() const { return ImplPtr->NetMode; }
void World::SetNetMode(ENetMode NewNetMode) { ImplPtr->NetMode = NewNetMode; }
bool World::IsStandalone() const { return ImplPtr->NetMode == ENetMode::Standalone; }
bool World::IsListenServer() const { return ImplPtr->NetMode == ENetMode::ListenServer; }
bool World::IsClient() const { return ImplPtr->NetMode == ENetMode::Client; }
bool World::IsServer() const { return IsStandalone() || IsListenServer(); }

FNetworkActorId World::AllocateNetworkActorId() {
  if (!IsServer()) {
    return 0;
  }
  return ImplPtr->NextNetworkActorId++;
}

bool World::ServerTravel(FNetworkSceneId SceneId) {
  if (!IsServer() || !SceneManager::GetInstance().IsSceneRegistered(SceneId)) {
    return false;
  }
  if (IsListenServer() && ImplPtr->ReplicationSystem) {
    ImplPtr->ReplicationSystem->BroadcastServerTravel(SceneId);
  }
  return SceneManager::GetInstance().OpenLevelById(SceneId, GetNetMode());
}

bool World::ServerTravel(const std::string& LevelPath) {
  if (!IsServer() || LevelPath.empty()) {
    return false;
  }
  if (IsListenServer() && ImplPtr->ReplicationSystem) {
    ImplPtr->ReplicationSystem->BroadcastServerTravel(LevelPath);
  }
  return SceneManager::GetInstance().OpenLevelByPath(LevelPath, GetNetMode());
}

APlayerController* World::GetOrCreateLocalPlayerController() {
  if (ImplPtr->LocalPlayerController && !ImplPtr->LocalPlayerController->IsPendingDestroy()) {
    return ImplPtr->LocalPlayerController;
  }
  AActor* controllerActor = nullptr;
  std::string controllerClass = ImplPtr->LocalPlayerControllerClass;
  if (controllerClass.empty() && ImplPtr->GameMode != nullptr) {
    controllerClass = ImplPtr->GameMode->GetDefaultPlayerControllerClass();
  }
  if (!controllerClass.empty()) {
    controllerActor = ActorRegistry::GetInstance().Spawn(this, controllerClass);
  }
  if (!controllerActor) {
    controllerActor = SpawnActor<APlayerController>();
  }
  ImplPtr->LocalPlayerController = dynamic_cast<APlayerController*>(controllerActor);
  if (!ImplPtr->LocalPlayerController) {
    if (controllerActor) {
      controllerActor->Destroy();
    }
    ImplPtr->LocalPlayerController = SpawnActor<APlayerController>();
  }
  ImplPtr->LocalPlayerController->SetPlayerId(0);
  ImplPtr->LocalPlayerController->bIsLocallyControlled = true;
  ImplPtr->LocalPlayerController->SetupInputMappings();
  ImplPtr->LocalPlayerController->Spawned();
  return ImplPtr->LocalPlayerController;
}

void World::SetLocalPlayerController(APlayerController* PC) {
  ImplPtr->LocalPlayerController = PC;
  if (ImplPtr->LocalPlayerController) {
    ImplPtr->LocalPlayerController->SetPlayerId(0);
    ImplPtr->LocalPlayerController->bIsLocallyControlled = true;
    ImplPtr->LocalPlayerController->SetupInputMappings();
  }
}

void World::PossessLocalPawn(APawn* Pawn) {
  if (!Pawn) {
    return;
  }
  if (ImplPtr->LocalPlayerController) {
    ImplPtr->LocalPlayerController->Possess(Pawn);
  }
}

void World::SetLocalPlayerControllerClass(const std::string& ClassName) {
  ImplPtr->LocalPlayerControllerClass = ClassName;
}
