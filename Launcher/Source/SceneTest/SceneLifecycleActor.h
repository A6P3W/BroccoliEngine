#pragma once

#include "Actor.h"
#include "TimerHandle.h"

// 3秒間隔で別のアクターを動的に生成・破棄（スポーン/デストロイ）し、アクターのライフサイクルを検証するアクター
class ASceneLifecycleActor : public AActor {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ASceneLifecycleActor)

  void BeginPlay() override;
  void OnUpdate(float DeltaTime) override;

 private:
  void ToggleSpawnedActor();
  // 生成されたアクターへのポインタ
  AActor* SpawnedActor = nullptr;
  // 生成・破棄ループの処理周期を管理するタイマーハンドル
  FTimerHandle LifecycleTimer;
};

class ASceneSpawnedMarkerActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ASceneSpawnedMarkerActor)
  ASceneSpawnedMarkerActor();
  void OnUpdate(float DeltaTime) override;
};
