#pragma once
#include <memory>
#include <string>

#include "ActorManager.h"
#include "BroccoliEngineAPI.h"
#include "CollisionSystem.h"
#include "Log.h"
#include "NetMode.h"
#include "NetworkTypes.h"
#include "SoundManager.h"
#include "TimerManager.h"
#include "UMath.h"

class AActor;
class AGameModeBase;
class FReplicationSystem;
class FPhysicsSystem3D;
class APlayerController;
class APawn;

/// @brief ワールドを管理するクラスです。
class BROCCOLI_ENGINE_API World {
 public:
  World();
  ~World();
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  /// @brief ワールドの更新関数。Draw関数の前に呼ばれる。
  /// @param DeltaTime 前回の呼び出しとの差
  void Update(float DeltaTime);
  /// @brief ワールドの描画関数。Update関数の後に呼ばれる。
  void Draw();

  FActorManager* GetActorManager();
  FCollisionSystem* GetCollisionSystem();
  FSoundManager* GetSoundManager();
  FTimerManager* GetTimerManager();
  FReplicationSystem* GetReplicationSystem();
  FPhysicsSystem3D* GetPhysicsSystem3D();

  /// @brief ワールドに設定されているゲームモードを取得します。
  /// @return AGameModeBaseクラス
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

  /// @brief ワールドのネットワークモードを取得します。
  /// @return
  ENetMode GetNetMode() const;
  /// @brief ワールドのネットワークモードを設定します。
  /// @param NewNetMode
  void SetNetMode(ENetMode NewNetMode);
  bool IsStandalone() const;
  bool IsListenServer() const;
  bool IsClient() const;
  bool IsServer() const;

  FNetworkActorId AllocateNetworkActorId();
  /// @brief 指定したレベルに接続中のクライアント全員で移動します。
  /// @param SceneId
  /// @return
  bool ServerTravel(FNetworkSceneId SceneId);
  /// @brief 指定したレベルに接続中のクライアント全員で移動します。
  /// @param LevelPath
  /// @return
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
