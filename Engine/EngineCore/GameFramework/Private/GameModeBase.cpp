#include "GameModeBase.h"

#include <vector>

#include "ActorRegistry.h"
#include "CameraComponent.h"
#include "CollisionSystem.h"
#include "ActorManager.h"
#include "Pawn.h"
#include "PlayerStart.h"
#include "TimerManager.h"

REGISTER_GAME_MODE(AGameModeBase)

struct AGameModeBase::Impl {
  std::string DefaultPawnClass;
  std::string DefaultPlayerControllerClass;
  APawn* PlayerPawn = nullptr;
  bool HostPlayerSpawned = false;
};

AGameModeBase::AGameModeBase() : ImplPtr(new Impl()) {
  ImplPtr->DefaultPawnClass = APawn::StaticClassName();
  ImplPtr->DefaultPlayerControllerClass = APlayerController::StaticClassName();
  auto* CameraComponent = NewObject<MCameraComponent>(this);
  if (CameraComponent) {
    CameraComponent->RegisterComponent();
    CameraComponent->SetActiveCamera();
  }
}

AGameModeBase::~AGameModeBase() { delete ImplPtr; }
APawn* AGameModeBase::GetPlayerPawn() const { return ImplPtr->PlayerPawn; }
const std::string& AGameModeBase::GetDefaultPlayerControllerClass() const { return ImplPtr->DefaultPlayerControllerClass; }
bool AGameModeBase::IsHostPlayerSpawned() const { return ImplPtr->HostPlayerSpawned; }
void AGameModeBase::SetPlayerPawn(APawn* Pawn) { ImplPtr->PlayerPawn = Pawn; }
void AGameModeBase::SetDefaultPawnClass(const std::string& ClassName) { ImplPtr->DefaultPawnClass = ClassName; }
void AGameModeBase::SetDefaultPlayerControllerClass(const std::string& ClassName) { ImplPtr->DefaultPlayerControllerClass = ClassName; }
void AGameModeBase::BeginPlay() {
  AActor::BeginPlay();
  if (GetWorld() && GetWorld()->IsServer() && !ImplPtr->HostPlayerSpawned) {
    ImplPtr->HostPlayerSpawned = SpawnDefaultPlayer(0) != nullptr;
  }
}

APlayerController* AGameModeBase::OnClientConnected(FNetworkConnectionId ConnectionId) {
  return SpawnDefaultPlayer(ConnectionId);
}

void AGameModeBase::OnClientDisconnected(FNetworkConnectionId ConnectionId) {
  if (!GetWorld() || !GetWorld()->GetObjectManager()) {
    return;
  }

  const auto& actors = GetWorld()->GetObjectManager()->GetAllActors();
  for (const auto& actorPtr : actors) {
    AActor* actor = actorPtr.get();
    if (!actor || actor->IsPendingDestroy()) {
      continue;
    }

    if (actor->OwnerConnectionId == ConnectionId) {
      if (dynamic_cast<APlayerController*>(actor) || dynamic_cast<APawn*>(actor)) {
        actor->Destroy();
        M_LOG(
            "Destroyed actor {} for disconnected client {}",
            actor->GetActorClassName(),
            ConnectionId
        );
      }
    }
  }
}

void AGameModeBase::OnUpdate(float DeltaTime) { (void)DeltaTime; }

void AGameModeBase::Draw() { AActor::Draw(); }

AActor* AGameModeBase::FindPlayerStart(FNetworkConnectionId ConnectionId) {
  if (!GetWorld() || !GetWorld()->GetObjectManager()) {
    return nullptr;
  }
  std::vector<APlayerStart*> playerStarts;
  const auto& actors = GetWorld()->GetObjectManager()->GetAllActors();
  for (const auto& actorPtr : actors) {
    if (!actorPtr || actorPtr->IsPendingDestroy()) {
      continue;
    }
    if (auto* playerStart = dynamic_cast<APlayerStart*>(actorPtr.get())) {
      playerStarts.push_back(playerStart);
    }
  }
  if (playerStarts.empty()) {
    return nullptr;
  }
  const size_t index = static_cast<size_t>(ConnectionId) % playerStarts.size();
  return playerStarts[index];
}

APlayerController* AGameModeBase::SpawnDefaultPlayer(FNetworkConnectionId ConnectionId) {
  if (!GetWorld()) {
    return nullptr;
  }
  AActor* playerStart = FindPlayerStart(ConnectionId);
  const FVector2D spawnLocation =
      playerStart ? playerStart->GetActorLocation() : FVector2D::ZeroVector();
  const FRotator spawnRotation = playerStart ? playerStart->GetActorRotation() : FRotator(0.0f);
  ActorRegistry& registry = ActorRegistry::GetInstance();
  APlayerController* controller = nullptr;

  if (ConnectionId == 0) {
    controller = GetWorld()->GetOrCreateLocalPlayerController();
  } else {
    AActor* controllerActor =
        registry.Spawn(GetWorld(), ImplPtr->DefaultPlayerControllerClass, spawnLocation, spawnRotation);
    controller = dynamic_cast<APlayerController*>(controllerActor);
    if (!controller) {
      M_LOG("SpawnDefaultPlayer failed: {} is not APlayerController", ImplPtr->DefaultPlayerControllerClass);
      if (controllerActor) {
        controllerActor->Destroy();
      }
      return nullptr;
    }
  }

  AActor* pawnActor = registry.Spawn(GetWorld(), ImplPtr->DefaultPawnClass, spawnLocation, spawnRotation);
  auto* pawn = dynamic_cast<APawn*>(pawnActor);
  if (!pawn) {
    M_LOG("SpawnDefaultPlayer failed: {} is not APawn", ImplPtr->DefaultPawnClass);
    if (pawnActor) {
      pawnActor->Destroy();
    }
    controller->Destroy();
    return nullptr;
  }

  const bool bIsLocalPlayer = (ConnectionId == 0);

  controller->OwnerConnectionId = ConnectionId;
  controller->bReplicates = true;
  controller->bHasAuthority = true;
  controller->bIsLocallyControlled = bIsLocalPlayer;
  controller->SetPlayerId(static_cast<int>(ConnectionId));
  controller->SetupInputMappings();

  pawn->OwnerConnectionId = ConnectionId;
  pawn->bReplicates = true;
  pawn->bHasAuthority = true;
  pawn->bIsLocallyControlled = bIsLocalPlayer;

  if (ConnectionId != 0) {
    controller->Spawned();
  }
  pawn->Spawned();
  ImplPtr->PlayerPawn = pawn;

  controller->Possess(pawn);

  M_LOG(
      "Spawned Player Pawn: {} at ({}, {})",
      pawn->GetActorClassName(),
      spawnLocation.X,
      spawnLocation.Y
  );

  OnPlayerSpawned(controller, pawn, ConnectionId);
  return controller;
}
