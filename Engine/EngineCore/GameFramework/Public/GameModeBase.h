#pragma once
#include "BroccoliEngineAPI.h"
#include <string>

#include "Actor.h"
#include "CollisionSystem.h"
#include "Log.h"
#include "NetworkTypes.h"
#include "ActorManager.h"
#include "PlayerController.h"
#include "TimerManager.h"
#include "UMath.h"
#include "World.h"

class AActor;
class APawn;
class APlayerController;

#define REGISTER_GAME_MODE(ClassName) \
  static TActorAutoRegister<ClassName> AutoRegister_##ClassName(true);

class BROCCOLI_ENGINE_API AGameModeBase : public AActor {
 public:
  DEFINE_ACTOR_CLASS(AGameModeBase)
  AGameModeBase();
  ~AGameModeBase() override;
  void BeginPlay() override;
  virtual void OnUpdate(float DeltaTime) override;
  virtual void Draw() override;

  virtual void OnPlayerSpawned(
      APlayerController* Controller, APawn* Pawn, FNetworkConnectionId ConnectionId
  ) {}

  APawn* GetPlayerPawn() const;
  const std::string& GetDefaultPlayerControllerClass() const;
  bool IsHostPlayerSpawned() const;
  virtual APlayerController* OnClientConnected(FNetworkConnectionId ConnectionId);
  virtual void OnClientDisconnected(FNetworkConnectionId ConnectionId);
  void BeginTravelWait();
  void OnClientTravelReady(FNetworkConnectionId ConnectionId);

  AActor* FindPlayerStart(FNetworkConnectionId ConnectionId);
  APlayerController* SpawnDefaultPlayer(FNetworkConnectionId ConnectionId);

  template <class T>
  T* SpawnActor(const FVector2D& Loc = {0, 0}, FRotator Rot = FRotator(0.0f)) {
    return GetWorld()->SpawnActor<T>(Loc, Rot);
  }

 protected:
  virtual void OnAllClientsTravelReady() {}

  void SetPlayerPawn(APawn* Pawn);
  void SetDefaultPawnClass(const std::string& ClassName);
  void SetDefaultPlayerControllerClass(const std::string& ClassName);

  template <class TPawn, class TController = APlayerController>
  TController* SpawnPlayer(const FVector2D& Location = FVector2D::ZeroVector(), int PlayerId = 0) {
    auto* Controller = GetWorld()->SpawnActor<TController>(Location);
    Controller->SetPlayerId(PlayerId);
    Controller->SetupInputMappings();

    auto* Pawn = GetWorld()->SpawnActor<TPawn>(Location);
    SetPlayerPawn(Pawn);
    Controller->Possess(Pawn);
    M_LOG("Spawned Player Pawn: {} at ({}, {})", Pawn->GetActorClassName(), Location.X, Location.Y);
    return Controller;
  }

 private:
  void CheckAllClientsTravelReady();

  struct Impl;
  Impl* ImplPtr = nullptr;
};
