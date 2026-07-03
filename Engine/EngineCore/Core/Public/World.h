#pragma once
#include <memory>
#include <string>

#include "CollisionSystem.h"
#include "Log.h"
#include "NetMode.h"
#include "NetworkTypes.h"
#include "ObjectManager.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "UMath.h"

class AActor;
class AGameModeBase;
class MReplicationSystem;
class APlayerController;
class APawn;

class World {
 public:
  World();
  ~World();

  void Update(float DeltaTime);
  void Draw();

  MObjectManager* GetObjectManager() { return ObjectManager.get(); }
  MCollisionSystem* GetCollisionSystem() { return CollisionSystem.get(); }
  MSoundManager* GetSoundManager() { return SoundManager.get(); }
  MTimerManager* GetTimerManager() { return TimerManager.get(); }
  MReplicationSystem* GetReplicationSystem() { return ReplicationSystem.get(); }

  AGameModeBase* GetGameMode() const { return GameMode; }

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
    return ObjectManager->SpawnObject<T>(Loc, Rot, DeferBeginPlay);
  }

  void SetSimulating(bool bRunning) { bSimulating = bRunning; }
  bool IsSimulating() const { return bSimulating; }
  bool IsTrendingDown() const { return bTrendingDown; }

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
  std::unique_ptr<MCollisionSystem> CollisionSystem = nullptr;
  std::unique_ptr<MSoundManager> SoundManager = nullptr;
  std::unique_ptr<MTimerManager> TimerManager = nullptr;
  std::unique_ptr<MReplicationSystem> ReplicationSystem = nullptr;
  std::unique_ptr<MObjectManager> ObjectManager = nullptr;

  AGameModeBase* GameMode = nullptr;
  bool bSimulating = true;
  bool bTrendingDown = false;
  ENetMode NetMode = ENetMode::Standalone;
  FNetworkActorId NextNetworkActorId = 1;

  APlayerController* LocalPlayerController = nullptr;
  std::string LocalPlayerControllerClass;
};
