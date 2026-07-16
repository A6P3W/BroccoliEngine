#pragma once
#include "BroccoliEngineAPI.h"
#include <memory>
#include <string>

#include "CollisionSystem.h"
#include "Log.h"
#include "NetMode.h"
#include "NetworkTypes.h"
#include "ActorManager.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "UMath.h"

class AActor;
class AGameModeBase;
class FReplicationSystem;
class APlayerController;
class APawn;

class BROCCOLI_ENGINE_API World {
 public:
  World();
  ~World();
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  void Update(float DeltaTime);
  void Draw();

  FActorManager* GetActorManager();
  FCollisionSystem* GetCollisionSystem();
  FSoundManager* GetSoundManager();
  FTimerManager* GetTimerManager();
  FReplicationSystem* GetReplicationSystem();

  AGameModeBase* GetGameMode() const;

  template <class T>
  T* SpawnGameMode() {
    T* mode = SpawnActor<T>({0, 0}, FRotator(0), true);
    SetGameMode(mode);
    mode->Spawned();
    return mode;
  }

  template <class T>
  T* SpawnActor(
      const FVector2D& Loc = {0, 0}, FRotator Rot = FRotator(0), bool DeferBeginPlay = false
  ) {
    return GetActorManager()->SpawnObject<T>(Loc, Rot, DeferBeginPlay);
  }

  void SetSimulating(bool bRunning);
  bool IsSimulating() const;
  bool IsTearingDown() const;
  void SetTargetFps(int NewTargetFps);
  int GetTargetFps() const;
  float GetCurrentFps() const;
  void UpdateCurrentFps(float DeltaTime);

  ENetMode GetNetMode() const;
  void SetNetMode(ENetMode NewNetMode);
  bool IsStandalone() const;
  bool IsListenServer() const;
  bool IsClient() const;
  bool IsServer() const;

  FNetworkActorId AllocateNetworkActorId();
  bool ServerTravel(FNetworkSceneId SceneId);
  bool ServerTravel(const std::string& LevelPath);
  void SetGameMode(AGameModeBase* mode);

  APlayerController* GetOrCreateLocalPlayerController();

  void SetLocalPlayerController(APlayerController* PC);

  void PossessLocalPawn(APawn* Pawn);
  void SetLocalPlayerControllerClass(const std::string& ClassName);

 private:
  struct Impl;
  Impl* ImplPtr = nullptr;
};
