#pragma once
#include "Actor.h"
#include "PlayerController.h"
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "TimerManager.h"
#include "Log.h"
#include "UMath.h"
#include "World.h"
#include "NetworkTypes.h"
#include <string>

class AActor;
class APawn;
class APlayerController;

#define REGISTER_GAME_MODE(ClassName) \
    static TActorAutoRegister<ClassName> AutoRegister_##ClassName(true);

class AGameModeBase : public AActor
{
public:
	DEFINE_ACTOR_CLASS(AGameModeBase)
	AGameModeBase();
	void BeginPlay() override;
	virtual void OnUpdate(float DeltaTime) override;
	virtual void Draw() override;

	virtual void OnPlayerSpawned(APlayerController* Controller, APawn* Pawn, FNetworkConnectionId ConnectionId) {}

	APawn* GetPlayerPawn() const { return PlayerPawn; }
	const std::string& GetDefaultPlayerControllerClass() const { return DefaultPlayerControllerClass; }

	bool IsHostPlayerSpawned() const { return bHostPlayerSpawned; }
	virtual APlayerController* OnClientConnected(FNetworkConnectionId ConnectionId);
	virtual void OnClientDisconnected(FNetworkConnectionId ConnectionId);

	AActor* FindPlayerStart(FNetworkConnectionId ConnectionId);
	APlayerController* SpawnDefaultPlayer(FNetworkConnectionId ConnectionId);

	template<class T>
	T* SpawnActor(const FVector2D& Loc = { 0,0 }, FRotator Rot = FRotator(0.0f)) {
		return GetWorld()->SpawnActor<T>(Loc, Rot);
	}
protected:
	std::string DefaultPawnClass;
	std::string DefaultPlayerControllerClass;

	template<class TPawn, class TController = APlayerController>
	TController* SpawnPlayer(const FVector2D& Location = FVector2D::ZeroVector, int PlayerId = 0)
	{
		auto* Controller = GetWorld()->SpawnActor<TController>(Location);
		Controller->SetPlayerId(PlayerId);
		Controller->SetupInputMappings();

		auto* Pawn = GetWorld()->SpawnActor<TPawn>(Location);
		PlayerPawn = Pawn;
		Controller->Possess(Pawn);
		M_LOG("Spawned Player Pawn: {} at ({}, {})", Pawn->GetActorClassName(), Location.X, Location.Y);
		return Controller;
	}

private:
	APawn* PlayerPawn = nullptr;
	bool bHostPlayerSpawned = false;
};
