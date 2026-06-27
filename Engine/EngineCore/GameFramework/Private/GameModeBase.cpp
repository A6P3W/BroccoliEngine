#include "GameModeBase.h"
#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "CameraComponent.h"
#include "ActorRegistry.h"
#include "PlayerStart.h"

#include <vector>

REGISTER_GAME_MODE(AGameModeBase)

AGameModeBase::AGameModeBase()
    : DefaultPawnClass(APawn::StaticClassName())
    , DefaultPlayerControllerClass(APlayerController::StaticClassName())
{
    auto camera = std::make_unique<MCameraComponent>();
    auto* camPtr = camera.get();
    this->AddComponent(std::move(camera)); 
    camPtr->SetActiveCamera();
}

APlayerController* AGameModeBase::OnClientConnected(FNetworkConnectionId ConnectionId)
{
    return SpawnDefaultPlayer(ConnectionId);
}

void AGameModeBase::OnClientDisconnected(FNetworkConnectionId ConnectionId)
{
    (void)ConnectionId;
}

void AGameModeBase::OnUpdate(float DeltaTime)
{
    (void)DeltaTime;

    if (GetWorld() && GetWorld()->IsServer() && !bHostPlayerSpawned) {
        bHostPlayerSpawned = SpawnDefaultPlayer(0) != nullptr;
    }
}

void AGameModeBase::Draw()
{
    AActor::Draw();
}

AActor* AGameModeBase::FindPlayerStart(FNetworkConnectionId ConnectionId)
{
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

APlayerController* AGameModeBase::SpawnDefaultPlayer(FNetworkConnectionId ConnectionId)
{
    if (!GetWorld()) {
        return nullptr;
    }

    AActor* playerStart = FindPlayerStart(ConnectionId);
    const FVector2D spawnLocation = playerStart ? playerStart->GetActorLocation() : FVector2D::ZeroVector;
    const FRotator spawnRotation = playerStart ? playerStart->GetActorRotation() : FRotator(0.0f);

    ActorRegistry& registry = ActorRegistry::GetInstance();
    AActor* controllerActor = registry.Spawn(GetWorld(), DefaultPlayerControllerClass, spawnLocation, spawnRotation);
    auto* controller = dynamic_cast<APlayerController*>(controllerActor);
    if (!controller) {
        M_LOG("SpawnDefaultPlayer failed: {} is not APlayerController", DefaultPlayerControllerClass);
        if (controllerActor) {
            controllerActor->Destroy();
        }
        return nullptr;
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
    controller->bHasAuthority = true;
    controller->bIsLocallyControlled = bIsLocalPlayer;
    controller->SetPlayerId(static_cast<int>(ConnectionId));
    controller->SetupInputMappings();

    pawn->OwnerConnectionId = ConnectionId;
    pawn->bReplicates = true;
    pawn->bHasAuthority = true;
    pawn->bIsLocallyControlled = bIsLocalPlayer;

    controller->Spawned();
    pawn->Spawned();

    PlayerPawn = pawn;
    controller->Possess(pawn);

    M_LOG("Spawned Player Pawn: {} at ({}, {})", pawn->GetActorClassName(), spawnLocation.X, spawnLocation.Y);
    return controller;
}
