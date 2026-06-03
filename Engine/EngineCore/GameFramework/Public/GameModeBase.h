#pragma once
#include "Actor.h"
#include "PlayerController.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "Utils/Log.h"
#include "Utils/UMath.h"
class APawn;
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

	ObjectManager* GetObjectManager() { return m_ObjectManager.get(); }
	CollisionSystem* GetCollisionSystem() { return m_CollisionSystem.get(); }
	TimerManager* GetTimerManager() { return m_TimerManager.get(); }

	template<class T>
	T* SpawnActor(const FVector2D& Loc = { 0,0 }, FRotator Rot = 0) {
		return m_ObjectManager->SpawnObject<T>(Loc, Rot);
	}
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

	std::unique_ptr<ObjectManager> m_ObjectManager = nullptr;
	std::unique_ptr<CollisionSystem> m_CollisionSystem = nullptr;
	std::unique_ptr<TimerManager> m_TimerManager = nullptr;
};

