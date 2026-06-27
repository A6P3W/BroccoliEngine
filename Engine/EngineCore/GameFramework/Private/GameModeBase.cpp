#include "GameModeBase.h"
#include "Pawn.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "CameraComponent.h"

REGISTER_GAME_MODE(AGameModeBase)

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
void AGameModeBase::OnUpdate(float DeltaTime)
{
}

void AGameModeBase::Draw()
{
    AActor::Draw();
}
