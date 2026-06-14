#pragma once
#include "Actor.h"
#include "PlayerController.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "Log.h"
#include "UMath.h"
#include "World.h"
class APlayerController;
class AGameModeBase : public AActor
{
public:
	DEFINE_ACTOR_CLASS(AGameModeBase)
		AGameModeBase();

	virtual void OnUpdate(float DeltaTime) override;
	virtual void Draw() override;

	APawn* GetPlayerPawn() const { return m_PlayerPawn; }
	APlayerController* GetPlayerController() const { return m_PlayerController; }

	template<class T>
	T* SpawnActor(const FVector2D& Loc = { 0,0 }, FRotator Rot = FRotator(0.0f)) {
		return GetWorld()->SpawnActor<T>(Loc, Rot);
	}
protected:

	template<class TPawn, class TController = APlayerController>
	TController* SpawnPlayer(const FVector2D& Location = FVector2D::ZeroVector, int PlayerId = 0)
	{
		auto* Controller = GetWorld()->SpawnActor<TController>(Location);
		m_PlayerController = Controller;
		Controller->SetPlayerId(PlayerId);
		Controller->SetupInputMappings();

		auto* Pawn = GetWorld()->SpawnActor<TPawn>(Location);
		m_PlayerPawn = Pawn;
		Controller->Possess(Pawn);
		M_LOG("Spawned Player Pawn: {} at ({}, {})", Pawn->GetActorClassName(), Location.X, Location.Y);
		return Controller;
	}

private:
	APawn* m_PlayerPawn;
	APlayerController* m_PlayerController;
};

