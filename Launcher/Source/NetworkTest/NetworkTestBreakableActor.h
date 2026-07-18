#pragma once

#include "Actor.h"

class MSpriteComponent;

// ネットワーク同期検証用の破壊可能なテストアクター
class ANetworkTestBreakableActor : public AActor {
 public:
  // クラスのメタデータを定義するエンジンマクロ
  DEFINE_ACTOR_CLASS(ANetworkTestBreakableActor)

  ANetworkTestBreakableActor();

 private:
  // 他のアクターとの衝突（重なり）検知時に呼び出されるコールバック関数
  void BeginOverlap(AActor* OtherActor) override;

  MSpriteComponent* BodySprite = nullptr;
};
