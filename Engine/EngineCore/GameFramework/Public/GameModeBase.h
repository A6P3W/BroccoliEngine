#pragma once
#include "Actor.h"
#include "PlayerController.h"
#include "ObjectManager.h"
#include "Utils/Log.h"
class APawn;
class APlayerController;
class AGameModeBase : public AActor
{
public:
    DEFINE_ACTOR_CLASS(AGameModeBase)
	AGameModeBase();

    APawn* GetPlayerPawn() const { return m_PlayerPawn; }
    APlayerController* GetPlayerController() const { return m_PlayerController; }
protected:

    template<class TPawn, class TController = APlayerController>
    TController* SpawnPlayer(const FVector2D& Location = FVector2D::ZeroVector, int PlayerId = 0)
    {
        auto* Controller = ObjectManager::GetInstance().SpawnObject<TController>();
		m_PlayerController = Controller;
        Controller->SetPlayerId(PlayerId);

        auto* Pawn = ObjectManager::GetInstance().SpawnObject<TPawn>(Location);
		m_PlayerPawn = Pawn;
        Controller->Possess(Pawn);
		M_LOG("Spawned Player Pawn: {} at ({}, {})", Pawn->GetActorClassName(), Location.X, Location.Y);
        return Controller;
    }

private:
    APawn* m_PlayerPawn;
    APlayerController* m_PlayerController;
};

