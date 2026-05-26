#pragma once
#include "Actor.h"
#include "PlayerController.h"
#include "ObjectManager.h"
class AGameModeBase : public AActor
{
public:
    DEFINE_ACTOR_CLASS(AGameModeBase)
	AGameModeBase();
protected:

    template<class TPawn, class TController = APlayerController>
    TController* SpawnPlayer(const FVector2D& Location = FVector2D::ZeroVector, int PlayerId = 0)
    {
        auto* Controller = ObjectManager::GetInstance().SpawnObject<TController>();
        Controller->SetPlayerId(PlayerId);

        auto* Pawn = ObjectManager::GetInstance().SpawnObject<TPawn>(Location);

        Controller->Possess(Pawn);

        return Controller;
    }

	AActor* m_PlayerPawn;
	AActor* m_PlayerController;
private:

};

