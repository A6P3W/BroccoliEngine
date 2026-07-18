#pragma once

#include "Actor.h"

class MSceneComponent;
class MSpriteComponent;

// コンポーネントのアタッチ規則（アタッチ時の座標・回転・スケールの扱い）をテストするためのアクター
class AAttachmentRuleTestActor : public AActor {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(AAttachmentRuleTestActor)

  AAttachmentRuleTestActor();

  void OnUpdate(float DeltaTime) override;

 private:
  // 回転の親となるコンポーネント
  MSceneComponent* RotatingParent = nullptr;

  // 各アタッチルール検証用のスプライトコンポーネント
  MSpriteComponent* KeepRelativeSprite = nullptr;
  MSpriteComponent* KeepWorldSprite = nullptr;
  MSpriteComponent* SnapSprite = nullptr;

  float RotationSpeed = 45.0f;
};
