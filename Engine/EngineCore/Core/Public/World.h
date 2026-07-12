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

  FActorManager* GetObjectManager() { return ActorManager.get(); }
  FCollisionSystem* GetCollisionSystem() { return CollisionSystem.get(); }
  FSoundManager* GetSoundManager() { return SoundManager.get(); }
  FTimerManager* GetTimerManager() { return TimerManager.get(); }
  FReplicationSystem* GetReplicationSystem() { return ReplicationSystem.get(); }

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
    return ActorManager->SpawnObject<T>(Loc, Rot, DeferBeginPlay);
  }

  void SetSimulating(bool bRunning) { bSimulating = bRunning; }
  bool IsSimulating() const { return bSimulating; }
  bool IsTearingDown() const { return bTearingDown; }
  void SetTargetFps(int NewTargetFps);
  int GetTargetFps() const { return TargetFps; }
  float GetCurrentFps() const { return CurrentFps; }
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
  std::unique_ptr<FCollisionSystem> CollisionSystem = nullptr;
  std::unique_ptr<FSoundManager> SoundManager = nullptr;
  std::unique_ptr<FTimerManager> TimerManager = nullptr;
  std::unique_ptr<FReplicationSystem> ReplicationSystem = nullptr;
  std::unique_ptr<FActorManager> ActorManager = nullptr;

  AGameModeBase* GameMode = nullptr;
  bool bSimulating = true;
  bool bTearingDown = false;
  int TargetFps = 60;
  float CurrentFps = 0.0f;
  ENetMode NetMode = ENetMode::Standalone;
  FNetworkActorId NextNetworkActorId = 1;

  APlayerController* LocalPlayerController = nullptr;
  std::string LocalPlayerControllerClass;
};
