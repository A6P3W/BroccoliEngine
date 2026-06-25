#include "GameModeBase.h"
#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "CameraComponent.h"

AGameModeBase::AGameModeBase()
{
    auto camera = std::make_unique<MCameraComponent>();
    auto* camPtr = camera.get();
    this->AddComponent(std::move(camera)); 
    camPtr->SetActiveCamera();
}

APlayerController* AGameModeBase::OnClientConnected(FNetworkConnectionId ConnectionId)
{
    APlayerController* controller = SpawnPlayer<APawn, APlayerController>(FVector2D::ZeroVector, static_cast<int>(ConnectionId));
    if (controller && controller->GetPawn()) {
        APawn* pawn = controller->GetPawn();
        pawn->OwnerConnectionId = ConnectionId;
        pawn->bReplicates = true;
        pawn->bHasAuthority = true;
        pawn->bIsLocallyControlled = false;
    }
    return controller;
}

void AGameModeBase::OnClientDisconnected(FNetworkConnectionId ConnectionId)
{
    (void)ConnectionId;
}
APlayerController* AGameModeBase::CreateLocalPlayerController()
{
    return GetWorld()->SpawnActor<APlayerController>(FVector2D::ZeroVector);
}
APlayerController* AGameModeBase::GetOrCreateLocalPlayerController()
{
    if (PlayerController) {
        return PlayerController;
    }

    PlayerController = CreateLocalPlayerController();
    PlayerController->SetPlayerId(0);
    PlayerController->SetupInputMappings();
    return PlayerController;
}

void AGameModeBase::PossessLocalPawn(APawn* Pawn)
{
    if (!Pawn) {
        return;
    }

    APlayerController* controller = GetOrCreateLocalPlayerController();
    if (controller) {
        PlayerPawn = Pawn;
        controller->Possess(Pawn);
    }
}

void AGameModeBase::OnUpdate(float DeltaTime)
{
}

void AGameModeBase::Draw()
{
    AActor::Draw();
}

