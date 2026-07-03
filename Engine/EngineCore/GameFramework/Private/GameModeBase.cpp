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

AGameModeBase::AGameModeBase()
    : DefaultPawnClass(APawn::StaticClassName()),
      DefaultPlayerControllerClass(APlayerController::StaticClassName()) {
  auto camera = std::make_unique<MCameraComponent>();
  auto* camPtr = camera.get();
  this->AddComponent(std::move(camera));
  camPtr->SetActiveCamera();
}

void AGameModeBase::BeginPlay() {
  AActor::BeginPlay();
  if (GetWorld() && GetWorld()->IsServer() && !bHostPlayerSpawned) {
    bHostPlayerSpawned = SpawnDefaultPlayer(0) != nullptr;
  }
}

APlayerController* AGameModeBase::OnClientConnected(FNetworkConnectionId ConnectionId) {
  return SpawnDefaultPlayer(ConnectionId);
}

void AGameModeBase::OnClientDisconnected(FNetworkConnectionId ConnectionId) { (void)ConnectionId; }

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
      playerStart ? playerStart->GetActorLocation() : FVector2D::ZeroVector;
  const FRotator spawnRotation = playerStart ? playerStart->GetActorRotation() : FRotator(0.0f);
  ActorRegistry& registry = ActorRegistry::GetInstance();
  APlayerController* controller = nullptr;

  if (ConnectionId == 0) {
    controller = GetWorld()->GetOrCreateLocalPlayerController();
  } else {
    AActor* controllerActor =
        registry.Spawn(GetWorld(), DefaultPlayerControllerClass, spawnLocation, spawnRotation);
    controller = dynamic_cast<APlayerController*>(controllerActor);
    if (!controller) {
      M_LOG("SpawnDefaultPlayer failed: {} is not APlayerController", DefaultPlayerControllerClass);
      if (controllerActor) {
        controllerActor->Destroy();
      }
      return nullptr;
    }
  }

  AActor* pawnActor = registry.Spawn(GetWorld(), DefaultPawnClass, spawnLocation, spawnRotation);
  auto* pawn = dynamic_cast<APawn*>(pawnActor);
  if (!pawn) {
    M_LOG("SpawnDefaultPlayer failed: {} is not APawn", DefaultPawnClass);
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
  PlayerPawn = pawn;

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
