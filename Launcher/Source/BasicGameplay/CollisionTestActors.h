#pragma once

#include "Actor.h"

class MCollisionComponent;

// エンジンの汎用オブジェクトであるアクターの基底クラス AActorを継承したコリジョン検証用ベースアクター
class ACollisionTestActorBase : public AActor {
 public:
  // 毎フレームの更新処理用ライフサイクル関数
  void OnUpdate(float DeltaTime) override;

  // このアクターのコリジョンが他アクター（OtherActor）と重なり始めた際にエンジンから呼び出されるコールバック関数
  void BeginOverlap(AActor* OtherActor) override;

  // 重なっていた他アクター（OtherActor）が離れた際にエンジンから呼び出されるコールバック関数
  void EndOverlap(AActor* OtherActor) override;

 protected:
  void ConfigureCollision(MCollisionComponent* Collision, const char* ShapeName);

 private:
  const char* ShapeLabel = "Collision";
  float ElapsedSeconds = 0.0f;
};

class ACollisionTestRectangleActor : public ACollisionTestActorBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ACollisionTestRectangleActor)
  ACollisionTestRectangleActor();
};

class ACollisionTestBlockRectangleActor : public ACollisionTestActorBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ACollisionTestBlockRectangleActor)
  ACollisionTestBlockRectangleActor();
};
class ACollisionTestCircleActor : public ACollisionTestActorBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ACollisionTestCircleActor)
  ACollisionTestCircleActor();
};

class ACollisionTestLineActor : public ACollisionTestActorBase {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ACollisionTestLineActor)
  ACollisionTestLineActor();
};
